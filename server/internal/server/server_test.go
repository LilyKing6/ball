package server

import (
	"encoding/json"
	"io"
	"net/http"
	"net/http/httptest"
	"strings"
	"testing"
	"time"

	"ballbattle-server/internal/proto"
	"ballbattle-server/internal/world"

	"github.com/gorilla/websocket"
)

func newTestHTTPServer(t *testing.T) *httptest.Server {
	t.Helper()

	hub := NewHub()

	// 测试房间不生成 AI，避免快照玩家数量受 AI 影响
	room := NewRoom("default", world.New(3000, 3000), hub)
	room.SetTargetAICount(0)
	hub.mu.Lock()
	hub.rooms["default"] = room
	hub.mu.Unlock()
	go room.Run()

	mux := http.NewServeMux()
	mux.HandleFunc("/ws", func(w http.ResponseWriter, r *http.Request) {
		ServeWSHTTP(hub, w, r)
	})
	mux.HandleFunc("/health", func(w http.ResponseWriter, r *http.Request) {
		w.WriteHeader(http.StatusOK)
		_, _ = w.Write([]byte("ok"))
	})

	server := httptest.NewServer(mux)
	t.Cleanup(func() {
		hub.mu.RLock()
		rooms := make([]*Room, 0, len(hub.rooms))
		for _, room := range hub.rooms {
			rooms = append(rooms, room)
		}
		hub.mu.RUnlock()

		for _, room := range rooms {
			room.Stop()
		}
		server.Close()
	})

	return server
}

func mustJSON(t *testing.T, value any) []byte {
	t.Helper()

	data, err := json.Marshal(value)
	if err != nil {
		t.Fatalf("marshal JSON: %v", err)
	}
	return data
}

func TestHealthEndpoint(t *testing.T) {
	server := newTestHTTPServer(t)

	response, err := http.Get(server.URL + "/health")
	if err != nil {
		t.Fatalf("GET /health: %v", err)
	}
	defer response.Body.Close()

	if response.StatusCode != http.StatusOK {
		t.Errorf("status = %d, want %d", response.StatusCode, http.StatusOK)
	}

	body, err := io.ReadAll(response.Body)
	if err != nil {
		t.Fatalf("read health response: %v", err)
	}
	if string(body) != "ok" {
		t.Errorf("body = %q, want %q", body, "ok")
	}
}

func TestWebSocketJoinReturnsWelcomeAndSnapshot(t *testing.T) {
	server := newTestHTTPServer(t)
	wsURL := "ws" + strings.TrimPrefix(server.URL, "http") + "/ws"

	conn, _, err := websocket.DefaultDialer.Dial(wsURL, nil)
	if err != nil {
		t.Fatalf("dial WebSocket: %v", err)
	}
	defer conn.Close()

	join := proto.Envelope{
		Type: "join",
		Payload: mustJSON(t, proto.JoinMsg{
			Name: "Alice",
			Mode: "free",
		}),
	}
	if err := conn.WriteMessage(websocket.TextMessage, mustJSON(t, join)); err != nil {
		t.Fatalf("send join: %v", err)
	}

	_ = conn.SetReadDeadline(time.Now().Add(2 * time.Second))
	_, raw, err := conn.ReadMessage()
	if err != nil {
		t.Fatalf("read welcome: %v", err)
	}
	var welcomeEnvelope proto.Envelope
	if err := json.Unmarshal(raw, &welcomeEnvelope); err != nil {
		t.Fatalf("unmarshal welcome envelope: %v", err)
	}
	if welcomeEnvelope.Type != "welcome" {
		t.Fatalf("first message type = %q, want %q", welcomeEnvelope.Type, "welcome")
	}
	var welcome proto.WelcomeMsg
	if err := json.Unmarshal(welcomeEnvelope.Payload, &welcome); err != nil {
		t.Fatalf("unmarshal welcome payload: %v", err)
	}
	if welcome.PlayerID <= 0 {
		t.Errorf("player ID = %d, want positive", welcome.PlayerID)
	}
	if welcome.RoomName != "default" {
		t.Errorf("room name = %q, want %q", welcome.RoomName, "default")
	}
	if welcome.WorldWidth != 3000 || welcome.WorldHeight != 3000 {
		t.Errorf("world size = %gx%g, want 3000x3000", welcome.WorldWidth, welcome.WorldHeight)
	}

	_ = conn.SetReadDeadline(time.Now().Add(2 * time.Second))
	_, raw, err = conn.ReadMessage()
	if err != nil {
		t.Fatalf("read snapshot: %v", err)
	}
	var snapshotEnvelope proto.Envelope
	if err := json.Unmarshal(raw, &snapshotEnvelope); err != nil {
		t.Fatalf("unmarshal snapshot envelope: %v", err)
	}
	if snapshotEnvelope.Type != "snapshot" {
		t.Fatalf("second message type = %q, want %q", snapshotEnvelope.Type, "snapshot")
	}
	var snapshotMessage proto.SnapshotMsg
	if err := json.Unmarshal(snapshotEnvelope.Payload, &snapshotMessage); err != nil {
		t.Fatalf("unmarshal snapshot payload: %v", err)
	}
	var snapshot world.Snapshot
	if err := json.Unmarshal(snapshotMessage.Snapshot, &snapshot); err != nil {
		t.Fatalf("unmarshal world snapshot: %v", err)
	}
	if snapshotMessage.TickID < 1 {
		t.Errorf("snapshot message tick ID = %d, want at least 1", snapshotMessage.TickID)
	}
	if len(snapshot.Players) != 1 {
		t.Fatalf("snapshot players = %d, want 1", len(snapshot.Players))
	}
	if snapshot.Players[0].ID != welcome.PlayerID {
		t.Errorf("snapshot player ID = %d, want %d", snapshot.Players[0].ID, welcome.PlayerID)
	}
}

func TestHubRejectsInputBeforeJoin(t *testing.T) {
	hub := NewHub()
	client := &Client{hub: hub, send: make(chan []byte, 1)}
	input := proto.Envelope{
		Type:    "input",
		Payload: mustJSON(t, proto.InputMsg{}),
	}

	hub.HandleMessage(client, mustJSON(t, input))

	select {
	case raw := <-client.send:
		var response proto.Envelope
		if err := json.Unmarshal(raw, &response); err != nil {
			t.Fatalf("unmarshal error envelope: %v", err)
		}
		if response.Type != "error" {
			t.Errorf("response type = %q, want %q", response.Type, "error")
		}
	default:
		t.Fatal("input before join did not queue an error envelope")
	}
}

func TestClientSendRawAfterCloseIsIgnored(t *testing.T) {
	client := &Client{send: make(chan []byte, 1)}
	client.closeSend()

	client.SendRaw([]byte(`late`))
}

func TestRoomSpawnsAIPlayers(t *testing.T) {
	hub := NewHub()
	room := NewRoom("test", world.New(1000, 1000), hub)
	room.SetTargetAICount(3)
	go room.Run()
	defer room.Stop()

	// 等待 AI 生成（Run 在 physTicker 触发后才会 ensureAICount）
	time.Sleep(100 * time.Millisecond)

	if n := room.AICount(); n != 3 {
		t.Errorf("AI count = %d, want 3", n)
	}
	if len(room.world.Players) != 3 {
		t.Errorf("world player count = %d, want 3", len(room.world.Players))
	}
	for _, p := range room.world.Players {
		if !p.IsAI {
			t.Errorf("player %d is not AI", p.ID)
		}
	}
}

func TestRoomMaintainsAICountAfterAIDeath(t *testing.T) {
	hub := NewHub()
	room := NewRoom("test", world.New(1000, 1000), hub)
	room.SetTargetAICount(2)
	go room.Run()
	defer room.Stop()

	time.Sleep(100 * time.Millisecond)
	if room.AICount() != 2 {
		t.Fatalf("initial AI count = %d, want 2", room.AICount())
	}

	// 手动杀死一个 AI，等待房间补充
	room.mu.Lock()
	for pid := range room.aiControllers {
		if p, ok := room.world.Players[pid]; ok {
			for i := range p.Cells {
				p.Cells[i].Alive = false
			}
		}
		break
	}
	room.mu.Unlock()

	time.Sleep(150 * time.Millisecond)

	if room.AICount() != 2 {
		t.Errorf("AI count after respawn = %d, want 2", room.AICount())
	}
}
