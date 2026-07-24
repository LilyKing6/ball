package proto

import "encoding/json"

// Vec2 二维向量
type Vec2 struct {
	X float64 `json:"x"`
	Y float64 `json:"y"`
}

// PlayerInput 玩家一帧输入
type PlayerInput struct {
	Cursor     Vec2 `json:"cursor"`
	WantSplit  bool `json:"wantSplit"`
	WantEject  bool `json:"wantEject"`
	MouseWorld Vec2 `json:"mouseWorld"`
}

// Envelope 所有消息统一外层结构（type + payload）
type Envelope struct {
	Type    string          `json:"type"`
	Payload json.RawMessage `json:"payload,omitempty"`
}

// JoinMsg 客户端请求加入房间
type JoinMsg struct {
	Name string `json:"name"`
	Mode string `json:"mode"`
}

// InputMsg 客户端发送的玩家输入
type InputMsg struct {
	Input PlayerInput `json:"input"`
}

// WelcomeMsg 服务端欢迎消息
type WelcomeMsg struct {
	PlayerID    int     `json:"playerId"`
	WorldWidth  float64 `json:"worldWidth"`
	WorldHeight float64 `json:"worldHeight"`
	RoomName    string  `json:"roomName"`
}

// ErrorMsg 错误消息
type ErrorMsg struct {
	Message string `json:"message"`
}

// SnapshotMsg 快照消息（payload 为 world.Snapshot 的 JSON）
type SnapshotMsg struct {
	TickID   int             `json:"tickId"`
	Snapshot json.RawMessage `json:"snapshot"`
}
