package server

import (
	"encoding/json"
	"sync"
	"time"

	"ballbattle-server/internal/proto"

	"github.com/gorilla/websocket"
)

// Client 表示一个 WebSocket 客户端会话
type Client struct {
	id         int
	conn       *websocket.Conn
	hub        *Hub
	room       *Room
	playerID   int
	name       string
	send       chan []byte
	sendMu     sync.RWMutex
	sendClosed bool
	ready      bool
}

const (
	writeWait      = 10 * time.Second
	pongWait       = 60 * time.Second
	pingPeriod     = (pongWait * 9) / 10
	maxMessageSize = 8192
)

func NewClient(hub *Hub, conn *websocket.Conn) *Client {
	return &Client{
		hub:  hub,
		conn: conn,
		send: make(chan []byte, 256),
	}
}

// SendRaw 把已序列化好的消息加入发送队列（线程安全）
// 慢消费者会被丢弃以避免阻塞
func (c *Client) SendRaw(data []byte) {
	c.sendMu.RLock()
	defer c.sendMu.RUnlock()
	if c.sendClosed {
		return
	}
	select {
	case c.send <- data:
	default:
		// 队列满，丢弃
	}
}

func (c *Client) closeSend() {
	c.sendMu.Lock()
	defer c.sendMu.Unlock()
	if c.sendClosed {
		return
	}
	c.sendClosed = true
	close(c.send)
}

// SendError 发送错误消息给客户端
func (c *Client) SendError(message string) {
	msg := proto.ErrorMsg{Message: message}
	payload, _ := json.Marshal(msg)
	env := proto.Envelope{Type: "error", Payload: payload}
	data, _ := json.Marshal(env)
	c.SendRaw(data)
}

// readPump 持续从 WebSocket 读消息，转交给 Hub 处理
func (c *Client) readPump() {
	defer func() {
		if c.room != nil {
			c.room.RemovePlayer(c)
		}
		c.hub.Unregister(c)
		c.closeSend()
		_ = c.conn.Close()
	}()

	c.conn.SetReadLimit(maxMessageSize)
	_ = c.conn.SetReadDeadline(time.Now().Add(pongWait))
	c.conn.SetPongHandler(func(string) error {
		_ = c.conn.SetReadDeadline(time.Now().Add(pongWait))
		return nil
	})

	for {
		_, raw, err := c.conn.ReadMessage()
		if err != nil {
			return
		}
		c.hub.HandleMessage(c, raw)
	}
}

// writePump 把 send chan 的消息写出去
func (c *Client) writePump() {
	ticker := time.NewTicker(pingPeriod)
	defer func() {
		ticker.Stop()
		_ = c.conn.Close()
	}()

	for {
		select {
		case message, ok := <-c.send:
			_ = c.conn.SetWriteDeadline(time.Now().Add(writeWait))
			if !ok {
				_ = c.conn.WriteMessage(websocket.CloseMessage, []byte{})
				return
			}
			if err := c.conn.WriteMessage(websocket.TextMessage, message); err != nil {
				return
			}
		case <-ticker.C:
			_ = c.conn.SetWriteDeadline(time.Now().Add(writeWait))
			if err := c.conn.WriteMessage(websocket.PingMessage, nil); err != nil {
				return
			}
		}
	}
}
