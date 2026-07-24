package world

import (
	"math"
	"testing"
)

func TestAIConfigForDifficulty(t *testing.T) {
	cfg := AIConfigForDifficulty(AIDifficultyHard)
	if cfg.PerceptionRadius != 1200 {
		t.Errorf("hard perception radius = %v, want 1200", cfg.PerceptionRadius)
	}
	if !cfg.CanSplit {
		t.Error("hard AI should be able to split")
	}
}

func TestAIWanderSetsCursor(t *testing.T) {
	w := newTestWorld()
	p := &Player{
		ID:     1,
		Cells:  []Cell{testCell(Vec2{X: 500, Y: 500}, 20)},
		Cursor: Vec2{X: 500, Y: 500},
	}
	w.Players[p.ID] = p

	ai := NewAIController(AIDifficultyEasy)
	ai.Update(p, Cfg.FixedDt, w)

	if p.Cursor.Sub(p.CenterOfMass()).Length() < 1 {
		t.Error("AI wander did not move cursor away from center")
	}
}

func TestAISeeksNearbyFood(t *testing.T) {
	w := newTestWorld()
	p := &Player{
		ID:     1,
		Cells:  []Cell{testCell(Vec2{X: 500, Y: 500}, 20)},
		Cursor: Vec2{X: 500, Y: 500},
	}
	w.Players[p.ID] = p
	w.Foods = []Food{
		{Pos: Vec2{X: 550, Y: 500}, Mass: Cfg.FoodMass, Alive: true},
	}

	ai := NewAIController(AIDifficultyNormal)
	// 强制让 AI 立即重新评估状态
	ai.reactionTimer = 0
	ai.Update(p, Cfg.FixedDt, w)

	if ai.state != AIStateSeekFood {
		t.Errorf("AI state = %v, want SeekFood", ai.state)
	}

	// cursor 应该靠近食物方向
	com := p.CenterOfMass()
	dir := p.Cursor.Sub(com).Normalized()
	foodDir := w.Foods[0].Pos.Sub(com).Normalized()
	dot := dir.X*foodDir.X + dir.Y*foodDir.Y
	if dot < 0.5 {
		t.Errorf("AI cursor direction deviates too much from food: dot = %v", dot)
	}
}

func TestAIFleesBiggerPlayer(t *testing.T) {
	w := newTestWorld()
	small := &Player{
		ID:     1,
		Cells:  []Cell{testCell(Vec2{X: 500, Y: 500}, 20)},
		Cursor: Vec2{X: 500, Y: 500},
	}
	big := &Player{
		ID:     2,
		Cells:  []Cell{testCell(Vec2{X: 550, Y: 500}, 200)},
		Cursor: Vec2{X: 550, Y: 500},
	}
	w.Players[small.ID] = small
	w.Players[big.ID] = big

	ai := NewAIController(AIDifficultyNormal)
	ai.reactionTimer = 0
	ai.Update(small, Cfg.FixedDt, w)

	if ai.state != AIStateFleeBigger {
		t.Errorf("AI state = %v, want FleeBigger", ai.state)
	}

	// cursor 应该远离大球
	com := small.CenterOfMass()
	cursorDir := small.Cursor.Sub(com).Normalized()
	threatDir := big.CenterOfMass().Sub(com).Normalized()
	dot := cursorDir.X*threatDir.X + cursorDir.Y*threatDir.Y
	if dot > -0.3 {
		t.Errorf("AI cursor is not fleeing away: dot = %v", dot)
	}
}

func TestAISplitKillChasesSmallerPlayer(t *testing.T) {
	w := newTestWorld()
	predator := &Player{
		ID:     1,
		Cells:  []Cell{testCell(Vec2{X: 500, Y: 500}, 100)},
		Cursor: Vec2{X: 500, Y: 500},
	}
	prey := &Player{
		ID:     2,
		Cells:  []Cell{testCell(Vec2{X: 560, Y: 500}, 20)},
		Cursor: Vec2{X: 560, Y: 500},
	}
	w.Players[predator.ID] = predator
	w.Players[prey.ID] = prey

	ai := NewAIController(AIDifficultyHard)
	// 冻结反应计时器，避免 scanEnvironment / evaluateState 覆盖手动状态
	ai.reactionTimer = 100
	ai.targetPlayerIdx = prey.ID
	ai.state = AIStateSplitKill

	// 猎物初始距离 450 > splitReach(400)，不应分裂
	prey.Cells[0].Pos = Vec2{X: 950, Y: 500}
	ai.Update(predator, Cfg.FixedDt, w)

	if predator.WantSplit {
		t.Error("predator should not split from too far away")
	}

	// 拉近到分裂射程内再更新
	prey.Cells[0].Pos = Vec2{X: 520, Y: 500}
	predator.WantSplit = false
	ai.Update(predator, Cfg.FixedDt, w)

	if !predator.WantSplit {
		t.Error("predator should want to split when prey is within split reach")
	}
}

func TestAIControllerAvoidsVirus(t *testing.T) {
	w := newTestWorld()
	p := &Player{
		ID:     1,
		Cells:  []Cell{testCell(Vec2{X: 500, Y: 500}, 20)},
		Cursor: Vec2{X: 500, Y: 500},
	}
	w.Players[p.ID] = p
	w.Viruses = []Virus{
		{Pos: Vec2{X: 505, Y: 500}, Mass: Cfg.VirusMass, Alive: true},
	}

	ai := NewAIController(AIDifficultyNormal)
	// 设置一个远离病毒的目标
	ai.targetPos = Vec2{X: 100, Y: 100}
	ai.state = AIStateWander
	ai.reactionTimer = 0
	ai.Update(p, Cfg.FixedDt, w)

	if !ai.virusNearby {
		t.Error("AI should detect nearby virus")
	}

	// cursor 应该远离病毒
	com := p.CenterOfMass()
	cursorDir := p.Cursor.Sub(com).Normalized()
	virusDir := w.Viruses[0].Pos.Sub(com).Normalized()
	dot := cursorDir.X*virusDir.X + cursorDir.Y*virusDir.Y
	if dot > 0.2 {
		t.Errorf("AI cursor should avoid virus: dot = %v", dot)
	}
}

func TestDefaultAIPlayerName(t *testing.T) {
	name := DefaultAIPlayerName(0)
	if name != "Alpha" {
		t.Errorf("name = %q, want Alpha", name)
	}
	name = DefaultAIPlayerName(len(aiNames))
	if name != "Alpha" {
		t.Errorf("wrapped name = %q, want Alpha", name)
	}
}

func TestRandomAIDifficultyDistribution(t *testing.T) {
	counts := make(map[AIDifficulty]int)
	for i := 0; i < 1000; i++ {
		counts[RandomAIDifficulty()]++
	}
	for _, diff := range []AIDifficulty{AIDifficultyEasy, AIDifficultyNormal, AIDifficultyHard} {
		if counts[diff] == 0 {
			t.Errorf("difficulty %v never appeared", diff)
		}
	}
}

func TestVec2Normalized(t *testing.T) {
	v := Vec2{3, 4}.Normalized()
	if math.Abs(v.Length()-1) > 1e-9 {
		t.Errorf("normalized length = %v, want 1", v.Length())
	}
}
