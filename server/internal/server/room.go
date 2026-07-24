package server

import (
	"encoding/json"
	"log"
	"sync"
	"time"

	"ballbattle-server/internal/proto"
	"ballbattle-server/internal/world"
)

// Room 一个游戏房间：World + 客户端列表 + tick loop
// M3 多房间：支持自定义容量与模式，空房间自动回收。
type Room struct {
	name    string
	mode    string
	capacity int
	world   *world.World
	hub     *Hub

	mu      sync.RWMutex
	clients map[int]*Client // playerID -> Client

	inputMu       sync.Mutex
	pendingInputs map[int]proto.PlayerInput

	aiMu           sync.Mutex
	aiControllers  map[int]*world.AIController
	nextAIID       int
	targetAICount  int

	tickID int
	stop   chan struct{}
}

// NewRoom 创建房间
func NewRoom(name string, w *world.World, hub *Hub) *Room {
	r := &Room{
		name:          name,
		mode:          "free",
		capacity:      20,
		world:         w,
		hub:           hub,
		clients:       make(map[int]*Client),
		pendingInputs: make(map[int]proto.PlayerInput),
		aiControllers: make(map[int]*world.AIController),
		nextAIID:      -1, // AI 使用负数 ID，避免与真人玩家正数 ID 冲突
		targetAICount: 10,
		stop:          make(chan struct{}),
	}
	return r
}

// SetMode 设置房间模式（启动前调用）
func (r *Room) SetMode(mode string) {
	r.aiMu.Lock()
	defer r.aiMu.Unlock()
	r.mode = mode
}

// SetCapacity 设置房间容量（启动前调用）
func (r *Room) SetCapacity(n int) {
	if n <= 0 {
		n = 20
	}
	r.aiMu.Lock()
	defer r.aiMu.Unlock()
	r.capacity = n
}

// Mode 返回房间模式
func (r *Room) Mode() string {
	r.aiMu.Lock()
	defer r.aiMu.Unlock()
	return r.mode
}

// Capacity 返回房间容量
func (r *Room) Capacity() int {
	r.aiMu.Lock()
	defer r.aiMu.Unlock()
	return r.capacity
}

// IsFull 房间是否已满（线程安全）
func (r *Room) IsFull() bool {
	r.mu.RLock()
	defer r.mu.RUnlock()
	return len(r.clients) >= r.capacity
}

// AddPlayer 客户端进入房间，返回分配的 playerID
func (r *Room) AddPlayer(c *Client, name string) int {
	r.mu.Lock()
	defer r.mu.Unlock()

	if name == "" {
		name = "Player"
	}
	// 用 client id 作为 player id，确保唯一
	pid := c.id
	r.world.AddPlayer(pid, name)
	c.playerID = pid
	c.room = r
	c.ready = false
	r.clients[pid] = c
	return pid
}

// RemovePlayer 客户端离开房间
func (r *Room) RemovePlayer(c *Client) {
	r.mu.Lock()
	defer r.mu.Unlock()
	if c.playerID > 0 {
		r.world.RemovePlayer(c.playerID)
		delete(r.clients, c.playerID)
		c.playerID = 0
		c.room = nil
		c.ready = false
	}
}

func (r *Room) markReady(c *Client) {
	r.mu.Lock()
	defer r.mu.Unlock()
	if c.room == r && r.clients[c.playerID] == c {
		c.ready = true
	}
}

// ApplyInput 接收玩家输入（线程安全）
func (r *Room) ApplyInput(playerID int, input proto.PlayerInput) {
	r.inputMu.Lock()
	defer r.inputMu.Unlock()
	r.pendingInputs[playerID] = input
}

// Stop 停止 tick loop
func (r *Room) Stop() {
	close(r.stop)
}

// SetTargetAICount 设置目标 AI 数量（可在房间启动前调用）
func (r *Room) SetTargetAICount(n int) {
	r.aiMu.Lock()
	defer r.aiMu.Unlock()
	r.targetAICount = n
}

// addAIPlayer 在 world 中添加一个 AI 玩家，并创建对应控制器
// 调用者必须持有 r.mu 和 r.aiMu 的写锁
func (r *Room) addAIPlayer() {
	pid := r.nextAIID
	r.nextAIID--

	name := world.DefaultAIPlayerName(-pid - 1)
	p := r.world.AddPlayer(pid, name)
	p.IsAI = true
	p.Team = 0

	diff := world.RandomAIDifficulty()
	r.aiControllers[pid] = world.NewAIController(diff)
}

// removeAIPlayer 移除一个 AI 玩家
// 调用者必须持有 r.mu 和 r.aiMu 的写锁
func (r *Room) removeAIPlayer(pid int) {
	r.world.RemovePlayer(pid)
	delete(r.aiControllers, pid)
}

// ensureAICount 维持目标数量的 AI 玩家
// AI 死亡后由 world.Step 自动复活；本函数只确保 AI 玩家总数 >= target。
// 调用者必须持有 r.mu 的写锁
func (r *Room) ensureAICount() {
	r.aiMu.Lock()
	defer r.aiMu.Unlock()

	target := r.targetAICount
	current := len(r.aiControllers)

	for current < target {
		r.addAIPlayer()
		current++
	}

	// 如果 AI 总数超过目标，移除死亡的 AI；如果仍超过则不动
	if current > target {
		for pid := range r.aiControllers {
			if p, ok := r.world.Players[pid]; ok && p.Dead {
				r.removeAIPlayer(pid)
				current--
				if current <= target {
					break
				}
			}
		}
	}
}

// updateAI 更新所有 AI 控制器，写入 cursor / WantSplit / WantEject
// 调用者必须持有 r.mu 的写锁
func (r *Room) updateAI(dt float64) {
	r.aiMu.Lock()
	defer r.aiMu.Unlock()

	for pid, ctrl := range r.aiControllers {
		p, ok := r.world.Players[pid]
		if !ok || p.Dead {
			continue
		}
		ctrl.Update(p, dt, r.world)
		// AIController 内部会直接设置 p.WantSplit / p.WantEject
		// world.Step 会消费并清空这些标志
	}
}

// Run tick loop（在独立 goroutine 中）
func (r *Room) Run() {
	// 启动时先补满 AI
	r.mu.Lock()
	r.ensureAICount()
	r.mu.Unlock()

	// 物理 60Hz
	physTicker := time.NewTicker(time.Second / 60)
	defer physTicker.Stop()

	// 广播 30Hz
	broadcastTicker := time.NewTicker(time.Second / 30)
	defer broadcastTicker.Stop()

	const dt = 1.0 / 60.0

	for {
		select {
		case <-r.stop:
			log.Printf("[Room %s] stopped", r.name)
			return

		case <-physTicker.C:
			r.mu.Lock()

			// 0) 更新 AI：写入 cursor / wantSplit / wantEject
			r.updateAI(dt)

			// 1) 收集本帧输入
			r.inputMu.Lock()
			inputs := r.pendingInputs
			r.pendingInputs = make(map[int]proto.PlayerInput)
			r.inputMu.Unlock()

			// 2) 应用输入到 world（cursor + wantSplit + wantEject）
			for pid, in := range inputs {
				r.world.SetCursor(pid, world.Vec2{X: in.Cursor.X, Y: in.Cursor.Y})
				if in.WantSplit {
					r.world.SetWantSplit(pid)
				}
				if in.WantEject {
					r.world.SetWantEject(pid)
				}
			}

			// 3) 推进物理
			r.world.Step(dt)
			r.tickID++

			// 4) 维持 AI 数量（AI 死亡后复活由 world.Step 处理，这里补充新的 AI）
			r.ensureAICount()

			// 5) 检测死亡并通知客户端
			for _, p := range r.world.Players {
				if p.Dead && !p.DeathNotified && !p.IsAI {
					p.DeathNotified = true
					if client, ok := r.clients[p.ID]; ok && client != nil {
						deathPayload, _ := json.Marshal(proto.ErrorMsg{Message: "You have been eaten!"})
						env := proto.Envelope{Type: "death", Payload: deathPayload}
						data, _ := json.Marshal(env)
						client.SendRaw(data)
					}
				}
			}
			r.mu.Unlock()

		case <-broadcastTicker.C:
			r.broadcast()
		}
	}
}

// broadcast 把当前 World 状态广播给所有客户端
func (r *Room) broadcast() {
	r.mu.RLock()
	if len(r.clients) == 0 {
		r.mu.RUnlock()
		return
	}
	// 拷贝客户端列表
	clients := make([]*Client, 0, len(r.clients))
	for _, c := range r.clients {
		if c.ready {
			clients = append(clients, c)
		}
	}
	r.mu.RUnlock()

	// 构造 snapshot
	r.mu.RLock()
	snap := r.world.BuildSnapshot(r.tickID)
	tickID := r.tickID
	r.mu.RUnlock()
	snapBytes, _ := json.Marshal(snap)

	wrapped := proto.SnapshotMsg{
		TickID:   tickID,
		Snapshot: snapBytes,
	}
	payload, _ := json.Marshal(wrapped)

	env := proto.Envelope{Type: "snapshot", Payload: payload}
	data, _ := json.Marshal(env)

	for _, c := range clients {
		c.SendRaw(data)
	}
}

// Info 获取房间列表信息（线程安全）
func (r *Room) Info() proto.RoomInfo {
	r.mu.RLock()
	defer r.mu.RUnlock()
	return proto.RoomInfo{
		Name:        r.name,
		Mode:        r.mode,
		PlayerCount: len(r.clients),
		Capacity:    r.capacity,
	}
}

// PlayerCount 当前真人玩家数（线程安全）
func (r *Room) PlayerCount() int {
	r.mu.RLock()
	defer r.mu.RUnlock()
	return len(r.clients)
}

// AICount 当前 AI 玩家数（线程安全）
func (r *Room) AICount() int {
	r.aiMu.Lock()
	defer r.aiMu.Unlock()
	return len(r.aiControllers)
}
