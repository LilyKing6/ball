package world

import "math"

// Virus 病毒实体
type Virus struct {
	Pos          Vec2
	Mass         float64
	Alive        bool
	RespawnTimer float64
}

// NewVirus 创建新病毒
func NewVirus(pos Vec2) Virus {
	return Virus{
		Pos:          pos,
		Mass:         Cfg.VirusMass,
		Alive:        true,
		RespawnTimer: Cfg.VirusRespawnTime,
	}
}

// Radius 病毒半径
func (v *Virus) Radius() float64 {
	return Cfg.RadiusConstant * math.Sqrt(v.Mass)
}

var _ = math.Sqrt
