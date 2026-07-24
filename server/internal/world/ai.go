package world

import (
	"math"
)

// AIState AI 行为状态
type AIState int

const (
	AIStateWander AIState = iota
	AIStateSeekFood
	AIStateChaseSmaller
	AIStateFleeBigger
	AIStateSplitKill
)

// AIDifficulty AI 难度
type AIDifficulty int

const (
	AIDifficultyEasy AIDifficulty = iota
	AIDifficultyNormal
	AIDifficultyHard
)

// AIConfig AI 难度配置
type AIConfig struct {
	PerceptionRadius float64
	ReactionDelay    float64
	CanSplit         bool
	SplitSkill       float64
}

// AIConfigForDifficulty 根据难度返回配置
func AIConfigForDifficulty(diff AIDifficulty) AIConfig {
	switch diff {
	case AIDifficultyEasy:
		return AIConfig{PerceptionRadius: 600, ReactionDelay: 0.5, CanSplit: false, SplitSkill: 0}
	case AIDifficultyNormal:
		return AIConfig{PerceptionRadius: 800, ReactionDelay: 0.2, CanSplit: true, SplitSkill: 0.3}
	case AIDifficultyHard:
		return AIConfig{PerceptionRadius: 1200, ReactionDelay: 0.05, CanSplit: true, SplitSkill: 0.8}
	}
	return AIConfig{PerceptionRadius: 800, ReactionDelay: 0.2, CanSplit: false, SplitSkill: 0}
}

// AIController 服务端 AI 控制器
// 直接修改对应 Player 的 Cursor、WantSplit、WantEject
// 不保存 Player 指针，避免并发问题：由 Room 在 Step 前调用 Update
// 此时 Room 持有 world 的写锁

type AIController struct {
	config AIConfig

	state           AIState
	reactionTimer   float64
	targetPos       Vec2
	targetFoodIdx   int
	targetPlayerIdx int
	threatPlayerIdx int
	nearestVirusPos Vec2
	virusNearby     bool
}

// NewAIController 创建 AI 控制器
func NewAIController(difficulty AIDifficulty) *AIController {
	return &AIController{
		config:        AIConfigForDifficulty(difficulty),
		state:         AIStateWander,
		reactionTimer: 0,
		targetPos:     Vec2{randFloat(0, 3000), randFloat(0, 3000)},
		targetFoodIdx: -1,
	}
}

// Update 推进 AI 一帧
// dt: 物理步长（秒）
// self: 当前 AI 控制的玩家
// world: 完整世界（只读访问，用于扫描环境）
func (ai *AIController) Update(self *Player, dt float64, w *World) {
	ai.reactionTimer -= dt

	if ai.reactionTimer <= 0 {
		ai.scanEnvironment(self, w)
		ai.evaluateState(self)
		ai.reactionTimer = ai.config.ReactionDelay
	}

	switch ai.state {
	case AIStateWander:
		ai.wander(self, w)
	case AIStateSeekFood:
		ai.seekFood(self, w)
	case AIStateChaseSmaller:
		ai.chaseSmaller(self, w)
	case AIStateFleeBigger:
		ai.fleeBigger(self, w)
	case AIStateSplitKill:
		ai.splitKill(self, w)
	}
}

func (ai *AIController) scanEnvironment(self *Player, w *World) {
	com := self.CenterOfMass()
	totalM := self.TotalMass()
	radius := ai.config.PerceptionRadius

	ai.targetFoodIdx = -1
	ai.targetPlayerIdx = -1
	ai.threatPlayerIdx = -1
	ai.virusNearby = false

	closestFoodDist := radius
	closestPreyDist := radius
	closestThreatDist := radius

	for i := range w.Foods {
		if !w.Foods[i].Alive {
			continue
		}
		d := w.Foods[i].Pos.Sub(com).Length()
		if d < closestFoodDist {
			closestFoodDist = d
			ai.targetFoodIdx = i
		}
	}

	for _, other := range w.Players {
		if other == self || other.Dead {
			continue
		}
		otherMass := other.TotalMass()
		if otherMass <= 0 {
			continue
		}
		otherCom := other.CenterOfMass()
		d := otherCom.Sub(com).Length()
		if d > radius {
			continue
		}
		if totalM > otherMass*Cfg.MassRatioForEat {
			if d < closestPreyDist {
				closestPreyDist = d
				ai.targetPlayerIdx = other.ID
			}
		} else {
			if d < closestThreatDist {
				closestThreatDist = d
				ai.threatPlayerIdx = other.ID
			}
		}
	}

	closestVirusDist := radius * 0.5
	for i := range w.Viruses {
		if !w.Viruses[i].Alive {
			continue
		}
		d := w.Viruses[i].Pos.Sub(com).Length()
		if d < closestVirusDist {
			closestVirusDist = d
			ai.nearestVirusPos = w.Viruses[i].Pos
			ai.virusNearby = true
		}
	}
}

func (ai *AIController) evaluateState(self *Player) {
	if ai.threatPlayerIdx >= 0 {
		ai.state = AIStateFleeBigger
		return
	}
	if ai.config.CanSplit && ai.targetPlayerIdx >= 0 && self.canSplit() {
		if randFloat(0, 1) < ai.config.SplitSkill {
			ai.state = AIStateSplitKill
			return
		}
	}
	if ai.targetPlayerIdx >= 0 {
		ai.state = AIStateChaseSmaller
		return
	}
	if ai.targetFoodIdx >= 0 {
		ai.state = AIStateSeekFood
		return
	}
	ai.state = AIStateWander
}

func (ai *AIController) moveToward(self *Player, target Vec2, w *World) {
	com := self.CenterOfMass()
	dir := target.Sub(com).Normalized()
	if dir.LengthSq() < 0.001 {
		dir = Vec2{1, 0}
	}

	if ai.virusNearby {
		away := com.Sub(ai.nearestVirusPos).Normalized()
		dir = dir.Add(away.Mul(0.5)).Normalized()
	}

	totalM := math.Max(self.TotalMass(), 10)
	offset := 8*math.Sqrt(totalM) + 100
	self.Cursor = com.Add(dir.Mul(offset))
}

func (ai *AIController) moveAwayFrom(self *Player, threat Vec2, w *World) {
	com := self.CenterOfMass()
	dir := com.Sub(threat).Normalized()
	if dir.LengthSq() < 0.001 {
		dir = Vec2{1, 0}
	}

	if ai.virusNearby {
		away := com.Sub(ai.nearestVirusPos).Normalized()
		dir = dir.Add(away.Mul(0.5)).Normalized()
	}

	totalM := math.Max(self.TotalMass(), 10)
	offset := 8*math.Sqrt(totalM) + 100
	self.Cursor = com.Add(dir.Mul(offset))
}

func (ai *AIController) wander(self *Player, w *World) {
	com := self.CenterOfMass()
	if ai.targetPos.Sub(com).Length() < 100 {
		ai.targetPos = Vec2{randFloat(50, w.Width-50), randFloat(50, w.Height-50)}
	}
	ai.moveToward(self, ai.targetPos, w)
}

func (ai *AIController) seekFood(self *Player, w *World) {
	if ai.targetFoodIdx < 0 || ai.targetFoodIdx >= len(w.Foods) {
		ai.wander(self, w)
		return
	}
	food := &w.Foods[ai.targetFoodIdx]
	if !food.Alive {
		ai.targetFoodIdx = -1
		ai.wander(self, w)
		return
	}
	ai.moveToward(self, food.Pos, w)
}

func (ai *AIController) playerByID(w *World, id int) *Player {
	for _, p := range w.Players {
		if p.ID == id {
			return p
		}
	}
	return nil
}

func (ai *AIController) chaseSmaller(self *Player, w *World) {
	prey := ai.playerByID(w, ai.targetPlayerIdx)
	if prey == nil || prey.Dead || prey.TotalMass() <= 0 {
		ai.targetPlayerIdx = -1
		ai.wander(self, w)
		return
	}
	ai.moveToward(self, prey.CenterOfMass(), w)
}

func (ai *AIController) fleeBigger(self *Player, w *World) {
	threat := ai.playerByID(w, ai.threatPlayerIdx)
	if threat == nil || threat.Dead || threat.TotalMass() <= 0 {
		ai.threatPlayerIdx = -1
		ai.wander(self, w)
		return
	}
	ai.moveAwayFrom(self, threat.CenterOfMass(), w)
}

func (ai *AIController) splitKill(self *Player, w *World) {
	prey := ai.playerByID(w, ai.targetPlayerIdx)
	if prey == nil || prey.Dead || prey.TotalMass() <= 0 || !self.canSplit() {
		ai.targetPlayerIdx = -1
		ai.wander(self, w)
		return
	}

	com := self.CenterOfMass()
	targetCom := prey.CenterOfMass()
	dir := targetCom.Sub(com).Normalized()
	if dir.LengthSq() < 0.001 {
		dir = Vec2{1, 0}
	}

	dist := targetCom.Sub(com).Length()
	splitReach := 800.0 * 0.5

	if dist < splitReach {
		self.Cursor = targetCom
		self.WantSplit = true
		ai.state = AIStateChaseSmaller
	} else {
		ai.moveToward(self, targetCom, w)
	}
}

// DefaultAIPlayerName 返回一个 AI 玩家名字
func DefaultAIPlayerName(index int) string {
	return aiNames[index%len(aiNames)]
}

var aiNames = []string{
	"Alpha", "Beta", "Gamma", "Delta", "Epsilon", "Zeta", "Eta", "Theta",
	"Iota", "Kappa", "Lambda", "Mu", "Nu", "Xi", "Omicron", "Pi",
	"Rho", "Sigma", "Tau", "Upsilon", "Phi", "Chi", "Psi", "Omega",
}

// RandomAIDifficulty 按权重随机返回难度
func RandomAIDifficulty() AIDifficulty {
	r := randFloat(0, 1)
	if r < 0.4 {
		return AIDifficultyEasy
	}
	if r < 0.8 {
		return AIDifficultyNormal
	}
	return AIDifficultyHard
}
