package world

import "math"

// Spore 被吐出的孢子
type Spore struct {
	Pos           Vec2
	Vel           Vec2
	Mass          float64
	Alive         bool
	OwnerID       int
	ImmunityTimer float64
	DecayTimer    float64
}

// NewSpore 创建新孢子
func NewSpore(pos, vel Vec2, mass float64, ownerID int) Spore {
	return Spore{
		Pos:           pos,
		Vel:           vel,
		Mass:          mass,
		Alive:         true,
		OwnerID:       ownerID,
		ImmunityTimer: Cfg.SporeImmunityTime,
		DecayTimer:    1.0,
	}
}

// Radius 孢子半径
func (s *Spore) Radius() float64 {
	return Cfg.RadiusConstant * math.Sqrt(s.Mass)
}

// Update 推进孢子物理（减速 + 边界钳制 + 衰减）
func (s *Spore) Update(dt float64, worldW, worldH float64) {
	if !s.Alive {
		return
	}
	if s.ImmunityTimer > 0 {
		s.ImmunityTimer -= dt
	}
	s.Vel = s.Vel.Mul(Cfg.SporeVelocityDecay)
	s.Pos = s.Pos.Add(s.Vel.Mul(dt))
	margin := s.Radius() + 5
	s.Pos.X = clamp(s.Pos.X, margin, worldW-margin)
	s.Pos.Y = clamp(s.Pos.Y, margin, worldH-margin)
	s.DecayTimer -= dt
	if s.DecayTimer <= 0 {
		s.Mass -= Cfg.SporeDecayRate
		if s.Mass < 1 {
			s.Alive = false
		}
		s.DecayTimer = 1.0
	}
}

var _ = math.Sqrt
