package server

import (
	"log"
	"net/http"

	"github.com/gorilla/websocket"
)

var upgrader = websocket.Upgrader{
	ReadBufferSize:  4096,
	WriteBufferSize: 4096,
	CheckOrigin: func(r *http.Request) bool {
		// M1 阶段允许任意 origin，方便本地开发
		return true
	},
}

// ServeWSHTTP HTTP 升级处理：将 HTTP 请求升级为 WebSocket
func ServeWSHTTP(hub *Hub, w http.ResponseWriter, r *http.Request) {
	conn, err := upgrader.Upgrade(w, r, nil)
	if err != nil {
		log.Printf("[ServeWS] upgrade failed: %v", err)
		return
	}
	ServeWS(hub, conn)
}
