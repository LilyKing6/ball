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
type Room struct {
	name  string
	world *world.World
	hub   *Hub

	mu      sync.RWMutex
	clients map[int]*Client // playerID -> Client

	inputMu       sync.Mutex
	pendingInputs map[int]proto.PlayerInput

	tickID int
	stop   chan struct{}
}

// NewRoom 创建房间
func NewRoom(name string, w *world.World, hub *Hub) *Room {
	return &Room{
		name:          name,
		world:         w,
		hub:           hub,
		clients:       make(map[int]*Client),
		pendingInputs: make(map[int]proto.PlayerInput),
		stop:          make(chan struct{}),
	}
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

// Run tick loop（在独立 goroutine 中）
func (r *Room) Run() {
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

			// 4) 检测死亡并通知客户端
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

// PlayerCount 当前玩家数（线程安全）
func (r *Room) PlayerCount() int {
	r.mu.RLock()
	defer r.mu.RUnlock()
	return len(r.clients)
}
