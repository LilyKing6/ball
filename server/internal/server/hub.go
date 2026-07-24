package server

import (
	"encoding/json"
	"log"
	"sync"
	"time"

	"ballbattle-server/internal/proto"
	"ballbattle-server/internal/world"

	"github.com/gorilla/websocket"
)

// Hub 管理所有 WebSocket 客户端 + 房间
// M3 多房间：支持创建/列出/加入多个房间，空房间自动回收。
type Hub struct {
	mu         sync.RWMutex
	rooms      map[string]*Room
	clients    map[*Client]bool
	nextClient int
}

// NewHub 创建 Hub
func NewHub() *Hub {
	h := &Hub{
		rooms:      make(map[string]*Room),
		clients:    make(map[*Client]bool),
		nextClient: 1,
	}
	// 启动后台清理 goroutine
	go h.cleanupLoop()
	return h
}

// DefaultRoom 获取/创建默认房间（保持旧客户端兼容）
func (h *Hub) DefaultRoom() *Room {
	return h.GetOrCreateRoom("default", "free", 20)
}

// GetOrCreateRoom 获取或创建指定房间
// 线程安全
func (h *Hub) GetOrCreateRoom(name, mode string, capacity int) *Room {
	if name == "" {
		name = "default"
	}
	h.mu.Lock()
	defer h.mu.Unlock()

	r, ok := h.rooms[name]
	if !ok {
		w := world.New(3000, 3000)
		r = NewRoom(name, w, h)
		r.SetMode(mode)
		r.SetCapacity(capacity)
		h.rooms[name] = r
		go r.Run()
		log.Printf("[Hub] room %s created and started (mode=%s, capacity=%d)", name, mode, capacity)
	}
	return r
}

// GetRoom 获取房间，不存在返回 nil
func (h *Hub) GetRoom(name string) *Room {
	h.mu.RLock()
	defer h.mu.RUnlock()
	return h.rooms[name]
}

// RoomList 返回当前房间列表（线程安全）
func (h *Hub) RoomList() []proto.RoomInfo {
	h.mu.RLock()
	defer h.mu.RUnlock()

	list := make([]proto.RoomInfo, 0, len(h.rooms))
	for _, r := range h.rooms {
		list = append(list, r.Info())
	}
	return list
}

// removeRoom 从 Hub 中移除房间（调用者需确保房间已停止）
// 由 cleanupLoop 调用
func (h *Hub) removeRoom(name string) {
	h.mu.Lock()
	defer h.mu.Unlock()
	delete(h.rooms, name)
	log.Printf("[Hub] room %s removed", name)
}

// cleanupLoop 定期检查并回收空房间
func (h *Hub) cleanupLoop() {
	ticker := time.NewTicker(30 * time.Second)
	defer ticker.Stop()
	for range ticker.C {
		h.mu.RLock()
		rooms := make([]*Room, 0, len(h.rooms))
		for name, r := range h.rooms {
			// 保留 default 房间，避免频繁创建销毁
			if name == "default" {
				continue
			}
			rooms = append(rooms, r)
		}
		h.mu.RUnlock()

		for _, r := range rooms {
			if r.PlayerCount() == 0 {
				r.Stop()
				h.removeRoom(r.name)
			}
		}
	}
}

// Register 登记新客户端
func (h *Hub) Register(c *Client) {
	h.mu.Lock()
	defer h.mu.Unlock()
	h.clients[c] = true
}

// Unregister 移除客户端
func (h *Hub) Unregister(c *Client) {
	h.mu.Lock()
	defer h.mu.Unlock()
	delete(h.clients, c)
}

// HandleMessage 处理一条客户端消息
func (h *Hub) HandleMessage(c *Client, raw []byte) {
	var env proto.Envelope
	if err := json.Unmarshal(raw, &env); err != nil {
		c.SendError("invalid json: " + err.Error())
		return
	}

	switch env.Type {
	case "join":
		var msg proto.JoinMsg
		if len(env.Payload) > 0 {
			_ = json.Unmarshal(env.Payload, &msg)
		}
		h.handleJoin(c, msg)

	case "create_room":
		var msg proto.CreateRoomMsg
		if err := json.Unmarshal(env.Payload, &msg); err != nil {
			c.SendError("bad create_room payload")
			return
		}
		h.handleCreateRoom(c, msg)

	case "list_rooms":
		h.handleListRooms(c)

	case "input":
		if c.room == nil {
			c.SendError("not in a room")
			return
		}
		var msg proto.InputMsg
		if err := json.Unmarshal(env.Payload, &msg); err != nil {
			c.SendError("bad input payload")
			return
		}
		c.room.ApplyInput(c.playerID, msg.Input)

	case "leave":
		if c.room != nil {
			c.room.RemovePlayer(c)
			c.room = nil
		}

	default:
		c.SendError("unknown message type: " + env.Type)
	}
}

func (h *Hub) handleJoin(c *Client, msg proto.JoinMsg) {
	// 已在房间：先退出
	if c.room != nil {
		c.room.RemovePlayer(c)
	}

	roomName := msg.Room
	if roomName == "" {
		roomName = "default"
	}

	var room *Room
	if msg.CreateIfMissing {
		capacity := msg.Capacity
		if capacity <= 0 {
			capacity = 20
		}
		room = h.GetOrCreateRoom(roomName, msg.Mode, capacity)
	} else {
		room = h.GetRoom(roomName)
		// 保持旧客户端兼容：default 房间不存在时自动创建
		if room == nil && roomName == "default" {
			room = h.GetOrCreateRoom(roomName, msg.Mode, 20)
		}
		if room == nil {
			c.SendError("room not found: " + roomName)
			return
		}
	}

	if room.IsFull() {
		c.SendError("room is full: " + roomName)
		return
	}

	playerID := room.AddPlayer(c, msg.Name)

	welcome := proto.WelcomeMsg{
		PlayerID:    playerID,
		WorldWidth:  room.world.Width,
		WorldHeight: room.world.Height,
		RoomName:    room.name,
	}
	payload, _ := json.Marshal(welcome)
	env := proto.Envelope{Type: "welcome", Payload: payload}
	data, _ := json.Marshal(env)
	c.SendRaw(data)
	room.markReady(c)
	log.Printf("[Hub] client %d joined room %s as player %d (name=%s)",
		c.id, room.name, playerID, msg.Name)
}

func (h *Hub) handleCreateRoom(c *Client, msg proto.CreateRoomMsg) {
	if msg.Name == "" {
		c.SendError("room name required")
		return
	}
	if h.GetRoom(msg.Name) != nil {
		c.SendError("room already exists: " + msg.Name)
		return
	}
	capacity := msg.Capacity
	if capacity <= 0 {
		capacity = 20
	}
	mode := msg.Mode
	if mode == "" {
		mode = "free"
	}
	room := h.GetOrCreateRoom(msg.Name, mode, capacity)

	created := proto.RoomCreatedMsg{
		Name:     room.name,
		Mode:     room.Mode(),
		Capacity: room.Capacity(),
	}
	payload, _ := json.Marshal(created)
	env := proto.Envelope{Type: "room_created", Payload: payload}
	data, _ := json.Marshal(env)
	c.SendRaw(data)
	log.Printf("[Hub] client %d created room %s (mode=%s, capacity=%d)",
		c.id, room.name, mode, capacity)
}

func (h *Hub) handleListRooms(c *Client) {
	list := proto.RoomListMsg{Rooms: h.RoomList()}
	payload, _ := json.Marshal(list)
	env := proto.Envelope{Type: "room_list", Payload: payload}
	data, _ := json.Marshal(env)
	c.SendRaw(data)
}

// ServeWS HTTP 升级到 WebSocket
func ServeWS(hub *Hub, conn *websocket.Conn) {
	c := NewClient(hub, conn)
	hub.mu.Lock()
	c.id = hub.nextClient
	hub.nextClient++
	hub.mu.Unlock()
	hub.Register(c)
	go c.writePump()
	c.readPump() // 阻塞，readPump 退出后清理
}
