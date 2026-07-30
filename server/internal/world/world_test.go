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

func testCell(pos Vec2, mass float64) Cell {
	return Cell{
		Pos:   pos,
		Mass:  mass,
		Alive: true,
	}
}

func requireClose(t *testing.T, got, want float64) {
	t.Helper()
	if math.Abs(got-want) > 1e-9 {
		t.Fatalf("got %v, want %v", got, want)
	}
}

func TestWorldStepSplitsCell(t *testing.T) {
	w := newTestWorld()
	pos := Vec2{X: 500, Y: 500}
	p := &Player{
		ID:        1,
		Cells:     []Cell{testCell(pos, 40)},
		Cursor:    Vec2{X: 600, Y: 500},
		WantSplit: true,
	}
	w.Players[p.ID] = p

	w.Step(Cfg.FixedDt)

	if len(p.Cells) != 2 {
		t.Fatalf("split produced %d cells, want 2", len(p.Cells))
	}
	requireClose(t, p.Cells[0].Mass, 20)
	requireClose(t, p.Cells[1].Mass, 20)
	requireClose(t, p.TotalMass(), 40)
	for i := range p.Cells {
		if !p.Cells[i].IsMerging {
			t.Errorf("cell %d is not marked as merging", i)
		}
	}
	if p.WantSplit {
		t.Error("WantSplit was not consumed")
	}
}

func TestWorldStepEjectsSpore(t *testing.T) {
	w := newTestWorld()
	pos := Vec2{X: 500, Y: 500}
	p := &Player{
		ID:        7,
		Cells:     []Cell{testCell(pos, 30)},
		Cursor:    Vec2{X: 600, Y: 500},
		WantEject: true,
	}
	w.Players[p.ID] = p

	w.Step(Cfg.FixedDt)

	if len(w.Spores) != 1 {
		t.Fatalf("eject produced %d spores, want 1", len(w.Spores))
	}
	spore := w.Spores[0]
	if !spore.Alive {
		t.Error("ejected spore is not alive")
	}
	if spore.OwnerID != 7 {
		t.Errorf("spore owner = %d, want 7", spore.OwnerID)
	}
	requireClose(t, spore.Mass, Cfg.EjectMass)
	requireClose(t, p.Cells[0].Mass, 16)
	if p.WantEject {
		t.Error("WantEject was not consumed")
	}
}

func TestWorldStepVirusCollisionFragmentsCell(t *testing.T) {
	w := newTestWorld()
	pos := Vec2{X: 500, Y: 500}
	p := &Player{
		ID:     1,
		Cells:  []Cell{testCell(pos, 220)},
		Cursor: pos,
	}
	w.Players[p.ID] = p
	w.Viruses = []Virus{NewVirus(pos)}

	w.Step(Cfg.FixedDt)

	if len(p.Cells) != Cfg.VirusFragmentCount {
		t.Fatalf("virus collision produced %d cells, want %d", len(p.Cells), Cfg.VirusFragmentCount)
	}
	requireClose(t, p.TotalMass(), 220+Cfg.VirusMass)
	if w.Viruses[0].Alive {
		t.Error("collided virus is still alive")
	}
}

func TestWorldStepDevoursSmallerPlayer(t *testing.T) {
	w := newTestWorld()
	pos := Vec2{X: 500, Y: 500}
	predator := &Player{
		ID:              1,
		Cells:           []Cell{testCell(pos, 100)},
		Cursor:          pos,
		InvincibleTimer: 0,
	}
	prey := &Player{
		ID:              2,
		Cells:           []Cell{testCell(pos, 10)},
		Cursor:          pos,
		InvincibleTimer: 0,
	}
	w.Players[predator.ID] = predator
	w.Players[prey.ID] = prey

	w.Step(Cfg.FixedDt)

	if !prey.Dead {
		t.Error("prey was not marked dead")
	}
	if prey.Cells[0].Alive {
		t.Error("prey cell is still alive")
	}
	requireClose(t, predator.Cells[0].Mass, 110)
}

func TestBuildSnapshotSerializesOnlyLiveEntities(t *testing.T) {
	w := newTestWorld()
	w.GameTime = 12.5
	w.TickID = 99
	w.Players[3] = &Player{
		ID:     3,
		Name:   "player",
		Team:   2,
		Shield: 1,
		Cells: []Cell{
			testCell(Vec2{X: 10, Y: 20}, 30),
			{Pos: Vec2{X: 30, Y: 40}, Mass: 5, Alive: false},
		},
	}
	w.Foods = []Food{
		{Pos: Vec2{X: 1, Y: 2}, Mass: Cfg.FoodMass, Alive: true},
		{Pos: Vec2{X: 3, Y: 4}, Mass: Cfg.FoodMass, Alive: false},
	}
	w.Spores = []Spore{
		{Pos: Vec2{X: 5, Y: 6}, Mass: 7, Alive: true},
		{Pos: Vec2{X: 7, Y: 8}, Mass: 9, Alive: false},
	}
	w.Viruses = []Virus{
		{Pos: Vec2{X: 9, Y: 10}, Mass: Cfg.VirusMass, Alive: true},
		{Pos: Vec2{X: 11, Y: 12}, Mass: Cfg.VirusMass, Alive: false},
	}
	w.BigBeans = []BigBean{
		{Pos: Vec2{X: 13, Y: 14}, Mass: 100, Alive: true},
		{Pos: Vec2{X: 15, Y: 16}, Mass: 200, Alive: false},
	}

	snapshot := w.BuildSnapshot(123)

	if snapshot.TickID != 123 {
		t.Errorf("snapshot tick ID = %d, want 123", snapshot.TickID)
	}
	requireClose(t, snapshot.GameTime, 12.5)
	if len(snapshot.Players) != 1 {
		t.Fatalf("snapshot has %d players, want 1", len(snapshot.Players))
	}
	player := snapshot.Players[0]
	if player.ID != 3 || player.Name != "player" || player.Team != 2 || player.Shield != 1 {
		t.Errorf("unexpected player observation: %+v", player)
	}
	if len(player.Cells) != 1 {
		t.Fatalf("snapshot player has %d cells, want 1", len(player.Cells))
	}
	requireClose(t, player.Cells[0].Mass, 30)
	if len(snapshot.Foods) != 1 {
		t.Errorf("snapshot has %d foods, want 1", len(snapshot.Foods))
	}
	if len(snapshot.Spores) != 1 {
		t.Errorf("snapshot has %d spores, want 1", len(snapshot.Spores))
	}
	if len(snapshot.Viruses) != 1 {
		t.Errorf("snapshot has %d viruses, want 1", len(snapshot.Viruses))
	}
	if len(snapshot.BigBeans) != 1 {
		t.Errorf("snapshot has %d big beans, want 1", len(snapshot.BigBeans))
	}
}

func TestBuildSnapshotCulledFiltersEntities(t *testing.T) {
	w := newTestWorld()
	w.Width = 1000
	w.Height = 1000

	// 玩家在 100,100（视野内）和 900,900（视野外）
	w.Players[1] = &Player{
		ID:     1,
		Name:   "nearby",
		Cells:  []Cell{testCell(Vec2{X: 100, Y: 100}, 30)},
		Cursor: Vec2{X: 100, Y: 100},
	}
	w.Players[2] = &Player{
		ID:     2,
		Name:   "faraway",
		Cells:  []Cell{testCell(Vec2{X: 900, Y: 900}, 30)},
		Cursor: Vec2{X: 900, Y: 900},
	}

	// 食物
	w.Foods = []Food{
		{Pos: Vec2{X: 110, Y: 110}, Mass: Cfg.FoodMass, Alive: true},
		{Pos: Vec2{X: 800, Y: 800}, Mass: Cfg.FoodMass, Alive: true},
	}

	// 病毒
	w.Viruses = []Virus{
		{Pos: Vec2{X: 120, Y: 120}, Mass: Cfg.VirusMass, Alive: true},
		{Pos: Vec2{X: 850, Y: 850}, Mass: Cfg.VirusMass, Alive: true},
	}

	// 大豆
	w.BigBeans = []BigBean{
		{Pos: Vec2{X: 130, Y: 130}, Mass: 100, Alive: true},
		{Pos: Vec2{X: 880, Y: 880}, Mass: 200, Alive: true},
	}

	// 以 (100,100) 为中心，半径 200 裁剪
	snap := w.BuildSnapshotCulled(5, 100, 100, 200)

	if len(snap.Players) != 1 {
		t.Fatalf("culled snapshot has %d players, want 1 (only nearby)", len(snap.Players))
	}
	if snap.Players[0].ID != 1 {
		t.Errorf("culled player ID = %d, want 1", snap.Players[0].ID)
	}
	if len(snap.Foods) != 1 {
		t.Errorf("culled snapshot has %d foods, want 1", len(snap.Foods))
	}
	if len(snap.Viruses) != 1 {
		t.Errorf("culled snapshot has %d viruses, want 1", len(snap.Viruses))
	}
	if len(snap.BigBeans) != 1 {
		t.Errorf("culled snapshot has %d big beans, want 1", len(snap.BigBeans))
	}
}

func TestBuildSnapshotCulledZeroRadiusReturnsFullSnapshot(t *testing.T) {
	w := newTestWorld()
	w.Players[1] = &Player{
		ID:     1,
		Name:   "p1",
		Cells:  []Cell{testCell(Vec2{X: 500, Y: 500}, 30)},
	}
	w.Foods = []Food{
		{Pos: Vec2{X: 100, Y: 100}, Mass: Cfg.FoodMass, Alive: true},
		{Pos: Vec2{X: 900, Y: 900}, Mass: Cfg.FoodMass, Alive: true},
	}

	// radius=0 应退化为全量快照
	snap := w.BuildSnapshotCulled(1, 500, 500, 0)

	if len(snap.Foods) != 2 {
		t.Errorf("full snapshot has %d foods, want 2", len(snap.Foods))
	}
	if len(snap.Players) != 1 {
		t.Errorf("full snapshot has %d players, want 1", len(snap.Players))
	}
}
