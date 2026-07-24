package world

import "math"

// BigBean 大豆实体
type BigBean struct {
	Pos   Vec2
	Mass  float64
	Alive bool
}

// NewBigBean 创建大豆
func NewBigBean(pos Vec2, minMass, maxMass float64) BigBean {
	return BigBean{
		Pos:   pos,
		Mass:  randFloat(minMass, maxMass),
		Alive: true,
	}
}

// Radius 大豆半径
func (b *BigBean) Radius() float64 {
	return Cfg.RadiusConstant * math.Sqrt(b.Mass)
}

var _ = math.Sqrt
