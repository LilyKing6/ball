package server

import (
	"encoding/json"
	"log"
	"sync"

	"ballbattle-server/internal/proto"
	"ballbattle-server/internal/world"

	"github.com/gorilla/websocket"
)

// Hub 管理所有 WebSocket 客户端 + 房间
type Hub struct {
	mu         sync.RWMutex
	rooms      map[string]*Room
	clients    map[*Client]bool
	nextClient int
}

// NewHub 创建 Hub
func NewHub() *Hub {
	return &Hub{
		rooms:      make(map[string]*Room),
		clients:    make(map[*Client]bool),
		nextClient: 1,
	}
}

// DefaultRoom 获取/创建默认房间（M1 只有一个自由模式房间）
func (h *Hub) DefaultRoom() *Room {
	h.mu.Lock()
	defer h.mu.Unlock()

	r, ok := h.rooms["default"]
	if !ok {
		w := world.New(3000, 3000)
		r = NewRoom("default", w, h)
		h.rooms["default"] = r
		go r.Run()
		log.Println("[Hub] default room created and started")
	}
	return r
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

	room := h.DefaultRoom()
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
