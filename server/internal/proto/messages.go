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
// Room 为空时服务端自动分配默认房间
// CreateIfMissing 为 true 且 Room 不存在时自动创建
// Capacity 仅对创建房间有效
// Mode 当前仅保留字段，服务端均使用自由模式

type JoinMsg struct {
	Name            string `json:"name"`
	Mode            string `json:"mode"`
	Room            string `json:"room,omitempty"`
	CreateIfMissing bool   `json:"createIfMissing,omitempty"`
	Capacity        int    `json:"capacity,omitempty"`
}

// CreateRoomMsg 客户端请求创建房间
type CreateRoomMsg struct {
	Name     string `json:"name"`
	Mode     string `json:"mode"`
	Capacity int    `json:"capacity,omitempty"`
}

// RoomInfo 房间列表条目
type RoomInfo struct {
	Name        string `json:"name"`
	Mode        string `json:"mode"`
	PlayerCount int    `json:"playerCount"`
	Capacity    int    `json:"capacity"`
}

// RoomListMsg 房间列表
type RoomListMsg struct {
	Rooms []RoomInfo `json:"rooms"`
}

// RoomCreatedMsg 房间创建成功
type RoomCreatedMsg struct {
	Name     string `json:"name"`
	Mode     string `json:"mode"`
	Capacity int    `json:"capacity"`
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

// RoomJoinedMsg 加入房间成功通知（兼容旧客户端：未收到 welcome 时可用）
type RoomJoinedMsg struct {
	RoomName string `json:"roomName"`
}

// LeaveRoomMsg 离开房间
type LeaveRoomMsg struct {
	Room string `json:"room,omitempty"`
}
