# Network Regression Baseline Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use `subagent-driven-development` (recommended) or `executing-plans` to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Establish repeatable automated coverage for the authoritative Go game server's core rules and WebSocket contract, then rebuild the server and desktop client from verified sources.

**Architecture:** Keep rule tests in the `world` package, where they can construct a deterministic `World` directly without starting timers or depending on random spawning. Keep protocol tests in the `server` package, launching an in-process HTTP/WebSocket server around `Hub`; this verifies the externally observable `join -> welcome -> snapshot` contract while retaining the production handlers.

**Tech Stack:** Go 1.26, standard-library `testing`/`httptest`, gorilla/websocket, CMake, MinGW, Qt 6.

**Repository note:** `D:\Projects\ball` is not currently a Git worktree, so the normal commit steps are replaced by explicit verification checkpoints. Do not initialize or rewrite Git history as part of this task.

**Execution status (2026-07-17):** Completed. Added world-rule and WebSocket regression tests, fixed room/world and send-queue races, rebuilt the Go server with `-buildvcs=false`, verified `/health` and the live `join -> welcome -> snapshot` flow, rebuilt the Qt target, and updated the migration documentation.

---

## Priority Context

This is the next task because it removes the highest-risk unfinished item: there is no automated test suite, while the server source has been edited after its checked-in executable was built. It must complete before adding server-side AI filling, multi-room/lobby support, account support, snapshot interpolation, or interest management.

The following remain later, independent work items:

1. Spawn and control server-side AI players for the network free-mode room.
2. Add room lifecycle/lobby and explicit mode selection; the existing server currently always routes joins to `default`.
3. Add client snapshot interpolation and server-side visibility culling.
4. Persist split-kill and virus-kill counters so their achievements can unlock.

## File Structure

| Path | Responsibility |
| --- | --- |
| `server/internal/world/world_test.go` | Deterministic unit tests for split, eject, virus fragmentation, devouring, and snapshot contents. |
| `server/internal/server/server_test.go` | In-process health and WebSocket integration tests for the production `Hub` and handlers. |
| `server/ballbattle-server.exe` | Rebuilt Go server artifact produced only after the test suite passes. |
| `build/BallBattle.exe` | Rebuilt Qt client artifact, confirming the client still links after verified server changes. |
| `docs/M1_NETWORK_MIGRATION.md` | Update its verification status and known limitations only after all commands below have succeeded. |

### Task 1: Add Deterministic World-Rule Tests

**Files:**

- Create: `server/internal/world/world_test.go`
- Modify: none
- Test: `server/internal/world/world_test.go`

- [ ] **Step 1: Write the failing world-rule tests**

Create `server/internal/world/world_test.go` with a manually constructed world. Do not call `New`, because `New` creates random entities. The helper must leave foods, spores, viruses, and big beans empty unless a test inserts the required entity.

```go
package world

import (
    "math"
    "testing"
)

func newTestWorld() *World {
    return &World{
        Width:    1000,
        Height:   1000,
        Players:  make(map[int]*Player),
        Foods:    []Food{},
        Spores:   []Spore{},
        Viruses:  []Virus{},
        BigBeans: []BigBean{},
    }
}

func testCell(x, y, mass float64) Cell {
    return Cell{Pos: Vec2{X: x, Y: y}, Mass: mass, Alive: true}
}

func requireClose(t *testing.T, got, want float64) {
    t.Helper()
    if math.Abs(got-want) > 1e-9 {
        t.Fatalf("got %v, want %v", got, want)
    }
}

func TestWorldStepSplitsCellAndConservesMass(t *testing.T) {
    w := newTestWorld()
    p := &Player{
        ID:        1,
        Cells:     []Cell{testCell(500, 500, 40)},
        Cursor:    Vec2{X: 800, Y: 500},
        WantSplit: true,
    }
    w.Players[p.ID] = p

    w.Step(Cfg.FixedDt)

    if len(p.Cells) != 2 {
        t.Fatalf("cell count = %d, want 2", len(p.Cells))
    }
    requireClose(t, p.TotalMass(), 40)
    requireClose(t, p.Cells[0].Mass, 20)
    requireClose(t, p.Cells[1].Mass, 20)
    if !p.Cells[0].IsMerging || !p.Cells[1].IsMerging {
        t.Fatal("both split cells must wait for merge cooldown")
    }
    if p.WantSplit {
        t.Fatal("split input must be consumed by one simulation step")
    }
}

func TestWorldStepEjectsSporeAndConservesMass(t *testing.T) {
    w := newTestWorld()
    p := &Player{
        ID:        7,
        Cells:     []Cell{testCell(500, 500, 30)},
        Cursor:    Vec2{X: 800, Y: 500},
        WantEject: true,
    }
    w.Players[p.ID] = p

    w.Step(Cfg.FixedDt)

    if len(w.Spores) != 1 {
        t.Fatalf("spore count = %d, want 1", len(w.Spores))
    }
    requireClose(t, p.TotalMass(), 16)
    requireClose(t, w.Spores[0].Mass, Cfg.EjectMass)
    if w.Spores[0].OwnerID != p.ID || !w.Spores[0].Alive {
        t.Fatal("ejected spore must be alive and owned by the ejecting player")
    }
    if p.WantEject {
        t.Fatal("eject input must be consumed by one simulation step")
    }
}

func TestWorldStepVirusFragmentsLargeCell(t *testing.T) {
    w := newTestWorld()
    p := &Player{ID: 1, Cells: []Cell{testCell(500, 500, 220)}, Cursor: Vec2{X: 500, Y: 500}}
    w.Players[p.ID] = p
    w.Viruses = []Virus{{Pos: Vec2{X: 500, Y: 500}, Mass: Cfg.VirusMass, Alive: true, RespawnTimer: Cfg.VirusRespawnTime}}

    w.Step(Cfg.FixedDt)

    if w.Viruses[0].Alive {
        t.Fatal("virus must be consumed by a sufficiently large cell")
    }
    if len(p.Cells) != Cfg.VirusFragmentCount {
        t.Fatalf("fragment count = %d, want %d", len(p.Cells), Cfg.VirusFragmentCount)
    }
    requireClose(t, p.TotalMass(), 220+Cfg.VirusMass)
}

func TestWorldStepDevoursSmallerPlayerAfterInvulnerability(t *testing.T) {
    w := newTestWorld()
    predator := &Player{ID: 1, Cells: []Cell{testCell(500, 500, 100)}, Cursor: Vec2{X: 500, Y: 500}}
    prey := &Player{ID: 2, Cells: []Cell{testCell(500, 500, 10)}, Cursor: Vec2{X: 500, Y: 500}}
    w.Players[predator.ID] = predator
    w.Players[prey.ID] = prey

    w.Step(Cfg.FixedDt)

    if !prey.Dead || prey.Cells[0].Alive {
        t.Fatal("overlapping smaller player must be dead after being devoured")
    }
    requireClose(t, predator.TotalMass(), 110)
}

func TestBuildSnapshotExcludesDeadEntitiesAndReportsLivePlayers(t *testing.T) {
    w := newTestWorld()
    w.GameTime = 12.5
    w.Players[1] = &Player{ID: 1, Name: "Alice", Cells: []Cell{testCell(100, 200, 10)}}
    w.Foods = []Food{{Pos: Vec2{X: 10, Y: 20}, Mass: 1, Alive: true}, {Alive: false}}
    w.Spores = []Spore{{Pos: Vec2{X: 30, Y: 40}, Mass: 14, Alive: true}, {Alive: false}}
    w.Viruses = []Virus{{Pos: Vec2{X: 50, Y: 60}, Mass: 100, Alive: true}, {Alive: false}}
    w.BigBeans = []BigBean{{Pos: Vec2{X: 70, Y: 80}, Mass: 100, Alive: true}, {Alive: false}}

    snap := w.BuildSnapshot(42)

    if snap.TickID != 42 || snap.GameTime != 12.5 || len(snap.Players) != 1 {
        t.Fatalf("unexpected snapshot header: %#v", snap)
    }
    if len(snap.Foods) != 1 || len(snap.Spores) != 1 || len(snap.Viruses) != 1 || len(snap.BigBeans) != 1 {
        t.Fatalf("snapshot included dead entities: %#v", snap)
    }
    if snap.Players[0].ID != 1 || snap.Players[0].Name != "Alice" || len(snap.Players[0].Cells) != 1 {
        t.Fatalf("unexpected player observation: %#v", snap.Players[0])
    }
}
```

- [ ] **Step 2: Format the new test file**

Run from `D:\Projects\ball\server`:

```powershell
gofmt -w internal\world\world_test.go
```

Expected: the file is reformatted in place with no diagnostic output.

- [ ] **Step 3: Run the tests and record the initial result**

Run from `D:\Projects\ball\server`:

```powershell
go test ./internal/world -run 'Test(WorldStep|BuildSnapshot)' -count=1 -v
```

Expected: the command compiles the new test file. Any failure must identify a mismatch in current rule behavior; do not relax an assertion without reconciling it with the matching production code in `world.go`, `player.go`, or `snapshot.go`.

- [ ] **Step 4: Correct only production behavior proven wrong by a failing test**

Do not refactor successful paths. If a test exposes a bug, make the smallest fix in the owning file:

```text
split/eject/merge rule               -> server/internal/world/player.go
world collision or respawn ordering  -> server/internal/world/world.go
serialized observations              -> server/internal/world/snapshot.go
```

Preserve these contract values while fixing:

```go
MassRatioForEat: 1.1
SplitMinMass: 36
EjectMass: 14
VirusFragmentCount: 9
MaxCellsPerPlayer: 16
```

- [ ] **Step 5: Run the world package under the race detector**

```powershell
go test -race ./internal/world -count=1
```

Expected: `ok   ballbattle-server/internal/world` with no race report.

- [ ] **Step 6: Record a logical checkpoint**

Because Git metadata is unavailable, record the exact passing command and the changed paths in the task handoff instead of attempting `git commit`.

### Task 2: Add HTTP and WebSocket Contract Tests

**Files:**

- Create: `server/internal/server/server_test.go`
- Modify: none
- Test: `server/internal/server/server_test.go`

- [ ] **Step 1: Write the failing integration tests around production handlers**

Create `server/internal/server/server_test.go`. The cleanup must stop each room created by the test so ticker goroutines do not leak into later tests.

```go
package server

import (
    "encoding/json"
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
    mux := http.NewServeMux()
    mux.HandleFunc("/health", func(w http.ResponseWriter, r *http.Request) {
        w.WriteHeader(http.StatusOK)
        _, _ = w.Write([]byte("ok"))
    })
    mux.HandleFunc("/ws", func(w http.ResponseWriter, r *http.Request) {
        ServeWSHTTP(hub, w, r)
    })
    srv := httptest.NewServer(mux)
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
        srv.Close()
    })
    return srv
}

func TestHealthEndpoint(t *testing.T) {
    srv := newTestHTTPServer(t)

    resp, err := http.Get(srv.URL + "/health")
    if err != nil {
        t.Fatal(err)
    }
    defer resp.Body.Close()
    if resp.StatusCode != http.StatusOK {
        t.Fatalf("status = %d, want %d", resp.StatusCode, http.StatusOK)
    }
}

func TestWebSocketJoinReturnsWelcomeAndSnapshot(t *testing.T) {
    srv := newTestHTTPServer(t)
    wsURL := "ws" + strings.TrimPrefix(srv.URL, "http") + "/ws"
    conn, _, err := websocket.DefaultDialer.Dial(wsURL, nil)
    if err != nil {
        t.Fatal(err)
    }
    defer conn.Close()
    _ = conn.SetReadDeadline(time.Now().Add(2 * time.Second))

    joinPayload, err := json.Marshal(proto.JoinMsg{Name: "Alice", Mode: "free"})
    if err != nil {
        t.Fatal(err)
    }
    if err := conn.WriteJSON(proto.Envelope{Type: "join", Payload: joinPayload}); err != nil {
        t.Fatal(err)
    }

    var welcomeEnvelope proto.Envelope
    if err := conn.ReadJSON(&welcomeEnvelope); err != nil {
        t.Fatal(err)
    }
    if welcomeEnvelope.Type != "welcome" {
        t.Fatalf("message type = %q, want welcome", welcomeEnvelope.Type)
    }
    var welcome proto.WelcomeMsg
    if err := json.Unmarshal(welcomeEnvelope.Payload, &welcome); err != nil {
        t.Fatal(err)
    }
    if welcome.PlayerID <= 0 || welcome.RoomName != "default" || welcome.WorldWidth != 3000 || welcome.WorldHeight != 3000 {
        t.Fatalf("unexpected welcome: %#v", welcome)
    }

    var snapshotEnvelope proto.Envelope
    if err := conn.ReadJSON(&snapshotEnvelope); err != nil {
        t.Fatal(err)
    }
    if snapshotEnvelope.Type != "snapshot" {
        t.Fatalf("message type = %q, want snapshot", snapshotEnvelope.Type)
    }
    var snapshotMessage proto.SnapshotMsg
    if err := json.Unmarshal(snapshotEnvelope.Payload, &snapshotMessage); err != nil {
        t.Fatal(err)
    }
    var snapshot world.Snapshot
    if err := json.Unmarshal(snapshotMessage.Snapshot, &snapshot); err != nil {
        t.Fatal(err)
    }
    if snapshot.TickID < 1 || len(snapshot.Players) != 1 || snapshot.Players[0].ID != welcome.PlayerID {
        t.Fatalf("unexpected snapshot: %#v", snapshot)
    }
}

func TestHubRejectsInputBeforeJoin(t *testing.T) {
    hub := NewHub()
    client := &Client{hub: hub, send: make(chan []byte, 1)}
    payload, err := json.Marshal(proto.InputMsg{})
    if err != nil {
        t.Fatal(err)
    }

    hub.HandleMessage(client, mustJSON(t, proto.Envelope{Type: "input", Payload: payload}))

    select {
    case raw := <-client.send:
        var envelope proto.Envelope
        if err := json.Unmarshal(raw, &envelope); err != nil {
            t.Fatal(err)
        }
        if envelope.Type != "error" {
            t.Fatalf("message type = %q, want error", envelope.Type)
        }
    default:
        t.Fatal("input before join must return an error response")
    }
}

func mustJSON(t *testing.T, value any) []byte {
    t.Helper()
    raw, err := json.Marshal(value)
    if err != nil {
        t.Fatal(err)
    }
    return raw
}
```

- [ ] **Step 2: Format the new integration test file**

Run from `D:\Projects\ball\server`:

```powershell
gofmt -w internal\server\server_test.go
```

Expected: the file is reformatted in place with no diagnostic output.

- [ ] **Step 3: Run the protocol tests and resolve only production defects**

Run from `D:\Projects\ball\server`:

```powershell
go test ./internal/server -run 'Test(HealthEndpoint|WebSocketJoinReturnsWelcomeAndSnapshot|HubRejectsInputBeforeJoin)' -count=1 -v
```

Expected: all three tests pass. If `TestWebSocketJoinReturnsWelcomeAndSnapshot` fails intermittently, fix lifecycle or locking in `hub.go`, `room.go`, or `client.go`; do not replace the asynchronous server test with a direct unit test.

- [ ] **Step 4: Run every Go package with race detection**

```powershell
go test -race ./... -count=1
```

Expected: all `ballbattle-server/...` packages report `ok`, with no data-race output.

- [ ] **Step 5: Record a logical checkpoint**

Capture the exact `go test -race ./... -count=1` result and changed paths. Do not create a fake commit in a non-Git directory.

### Task 3: Rebuild Artifacts and Perform a Focused Smoke Test

**Files:**

- Modify: `server/ballbattle-server.exe` (build output)
- Modify: `build/BallBattle.exe` (build output)
- Modify: `docs/M1_NETWORK_MIGRATION.md` (verification status only)
- Test: Go tests, server health endpoint, desktop build

- [ ] **Step 1: Rebuild the server only after all Go tests pass**

Run from `D:\Projects\ball\server`:

```powershell
go build -o ballbattle-server.exe .
```

Expected: exit code 0 and a `server/ballbattle-server.exe` timestamp newer than `server/internal/server/room.go`.

- [ ] **Step 2: Start the rebuilt server and verify health**

In one PowerShell terminal from `D:\Projects\ball\server`:

```powershell
.\ballbattle-server.exe -addr :8765
```

In a second terminal:

```powershell
Invoke-WebRequest http://127.0.0.1:8765/health | Select-Object -ExpandProperty Content
```

Expected output: `ok`.

- [ ] **Step 3: Verify the live WebSocket path with the existing manual client**

With the server from Step 2 still running, run from `D:\Projects\ball`:

```powershell
python tools\m1_test_client.py
```

Expected: a `welcome` line containing a positive player ID and recurring `snapshot` output. End the client with `Ctrl+C`, then end the server with `Ctrl+C`.

- [ ] **Step 4: Rebuild the Qt desktop client using the existing configured build directory**

Run from `D:\Projects\ball`:

```powershell
cmake --build build --parallel
```

Expected: exit code 0 and `build/BallBattle.exe` rebuilt successfully. Do not reconfigure CMake in this task; the configured environment currently uses MinGW and Qt 6.11.1.

- [ ] **Step 5: Update the migration document with verified facts**

In `docs/M1_NETWORK_MIGRATION.md`, replace claims that server-side split, eject, merge, virus, and big-bean behavior are unimplemented with a dated verification note. Retain these still-true limitations: fixed `default` room, no account system, no server-side AI player generation, full-world snapshots, and no interpolation.

Use this exact note beneath the current-progress section:

```markdown
> Verification update (2026-07-16): Go world-rule and WebSocket contract tests pass under `go test -race ./... -count=1`. The server implements split, eject, merge, virus fragmentation, spores, and big beans in free mode. It still exposes one fixed `default` room, has no account system or AI player generation, and broadcasts full snapshots without client interpolation.
```

- [ ] **Step 6: Final verification checkpoint**

Run these commands from the stated directories and preserve their output in the task handoff:

```powershell
# D:\Projects\ball\server
go test -race ./... -count=1

# D:\Projects\ball
cmake --build build --parallel
```

Expected: both exit with code 0. The next implementation task may begin only after this checkpoint is green.

## Plan Self-Review

- **Spec coverage:** The plan covers the highest-priority unfinished item, automated regression verification; it tests authoritative game mechanics, the HTTP health endpoint, join/welcome/snapshot protocol behavior, invalid input sequencing, server rebuild, live smoke test, client build, and documentation reconciliation.
- **Deliberate scope exclusions:** Server AI, multi-room/lobby, authentication, client interpolation/visibility culling, and achievement counters are listed as later work so this plan remains independently shippable.
- **Type consistency:** Tests use existing `World`, `Player`, `Cell`, `Spore`, `Virus`, `BigBean`, `proto.Envelope`, `proto.SnapshotMsg`, `Hub`, `Room`, and `Client` APIs. No production API is introduced merely to support testing.
- **Placeholder scan:** Every test includes concrete inputs and expected assertions; every verification step provides an exact command and success criterion.
