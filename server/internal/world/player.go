package world

import "math"

// Vec2 二维向量
type Vec2 struct {
	X, Y float64
}

func (v Vec2) Add(o Vec2) Vec2      { return Vec2{v.X + o.X, v.Y + o.Y} }
func (v Vec2) Sub(o Vec2) Vec2      { return Vec2{v.X - o.X, v.Y - o.Y} }
func (v Vec2) Mul(s float64) Vec2   { return Vec2{v.X * s, v.Y * s} }
func (v Vec2) Length() float64      { return math.Sqrt(v.X*v.X + v.Y*v.Y) }
func (v Vec2) LengthSq() float64    { return v.X*v.X + v.Y*v.Y }
func (v Vec2) Normalized() Vec2 {
	l := v.Length()
	if l < 1e-9 {
		return Vec2{1, 0}
	}
	return Vec2{v.X / l, v.Y / l}
}

func clamp(v, lo, hi float64) float64 {
	if v < lo {
		return lo
	}
	if v > hi {
		return hi
	}
	return v
}

// Cell 单个细胞
type Cell struct {
	Pos           Vec2
	Vel           Vec2
	Mass          float64
	Alive         bool
	SplitCooldown float64 // 分裂冷却剩余秒数
	IsMerging     bool    // 融合中
	MergeTimer    float64 // 融合倒计时
}

func (c *Cell) Radius() float64 {
	return Cfg.RadiusConstant * math.Sqrt(c.Mass)
}

// Player 玩家（包含多个 cell）
type Player struct {
	ID              int
	Name            string
	Team            int
	IsAI            bool
	Cells           []Cell
	Cursor          Vec2   // 虚拟游标位置（世界坐标）
	Shield          int    // 防护盾数量
	InvincibleTimer float64 // 出生无敌剩余秒数
	WantSplit       bool   // 由 Room 从 input 写入，Step 消费后清空
	WantEject       bool   // 同上
	Dead            bool   // 所有 cell 死亡后设为 true
	DeathNotified   bool   // 死亡通知已发送
	RespawnTimer    float64 // AI 复活倒计时
}

func NewPlayer(id int, name string, worldW, worldH float64) *Player {
	c := Cell{
		Pos:   Vec2{randFloat(100, worldW-100), randFloat(100, worldH-100)},
		Vel:   Vec2{},
		Mass:  Cfg.InitialMass,
		Alive: true,
	}
	return &Player{
		ID:    id,
		Name:  name,
		Cells: []Cell{c},
		Cursor: c.Pos,
		InvincibleTimer: 3.0,
	}
}

// TotalMass 玩家所有存活 cell 的总质量
func (p *Player) TotalMass() float64 {
	sum := 0.0
	for i := range p.Cells {
		if p.Cells[i].Alive {
			sum += p.Cells[i].Mass
		}
	}
	return sum
}

// CenterOfMass 质心
func (p *Player) CenterOfMass() Vec2 {
	var c Vec2
	total := 0.0
	for i := range p.Cells {
		if !p.Cells[i].Alive {
			continue
		}
		c = c.Add(p.Cells[i].Pos.Mul(p.Cells[i].Mass))
		total += p.Cells[i].Mass
	}
	if total > 0 {
		return c.Mul(1.0 / total)
	}
	return c
}

// IsDead 所有 cell 都死了
func (p *Player) IsDead() bool {
	for i := range p.Cells {
		if p.Cells[i].Alive {
			return false
		}
	}
	return true
}

// canSplit 是否可分裂
func (p *Player) canSplit() bool {
	if len(p.Cells) >= Cfg.MaxCellsPerPlayer {
		return false
	}
	for i := range p.Cells {
		c := &p.Cells[i]
		if c.Alive && c.Mass >= Cfg.SplitMinMass && c.SplitCooldown <= 0 {
			return true
		}
	}
	return false
}

// split 对每个符合条件的 cell 进行分裂
func (p *Player) split(worldW, worldH float64) []Cell {
	var newCells []Cell
	for i := range p.Cells {
		c := &p.Cells[i]
		if !c.Alive || c.Mass < Cfg.SplitMinMass || c.SplitCooldown > 0 {
			continue
		}
		// 均分质量
		halfMass := c.Mass / 2.0
		c.Mass = halfMass

		// 分裂方向（游标方向；若无有效方向则随机）
		dir := p.Cursor.Sub(c.Pos).Normalized()

		// 新 cell 沿方向弹出
		nc := Cell{
			Pos:        c.Pos.Add(dir.Mul(c.Radius() + 8)),
			Vel:        dir.Mul(Cfg.SplitVelocity),
			Mass:       halfMass,
			Alive:      true,
			IsMerging:  true,
			MergeTimer: Cfg.MergeCooldown,
		}
		// 原 cell 也进入融合状态
		c.IsMerging = true
		c.MergeTimer = Cfg.MergeCooldown
		c.SplitCooldown = Cfg.SplitCooldown

		// 边界钳制
		nc.Pos.X = clamp(nc.Pos.X, nc.Radius(), worldW-nc.Radius())
		nc.Pos.Y = clamp(nc.Pos.Y, nc.Radius(), worldH-nc.Radius())

		newCells = append(newCells, nc)

		if len(p.Cells)+len(newCells) >= Cfg.MaxCellsPerPlayer {
			break
		}
	}
	return newCells
}

// eject 对每个符合条件的 cell 吐球
func (p *Player) eject(worldW, worldH float64) []Spore {
	var spores []Spore
	for i := range p.Cells {
		c := &p.Cells[i]
		if !c.Alive || c.Mass < Cfg.EjectMass+10 {
			continue
		}
		c.Mass -= Cfg.EjectMass

		dir := p.Cursor.Sub(c.Pos).Normalized()

		s := NewSpore(
			c.Pos.Add(dir.Mul(c.Radius() + 5)),
			dir.Mul(Cfg.EjectVelocity),
			Cfg.EjectMass,
			p.ID,
		)
		spores = append(spores, s)

		if len(spores) >= 3 { // 每帧最多吐 3 个
			break
		}
	}
	return spores
}

// updateMerge 处理细胞融合
func (p *Player) updateMerge(dt float64) {
	// 倒计时
	for i := range p.Cells {
		c := &p.Cells[i]
		if !c.Alive || !c.IsMerging {
			continue
		}
		c.MergeTimer -= dt
		if c.MergeTimer <= 0 {
			c.IsMerging = false
			c.MergeTimer = 0
		}
	}

	// 融合检测：重叠 >= 85% 的已结束冷却的 cell 合并
	for i := 0; i < len(p.Cells); i++ {
		ca := &p.Cells[i]
		if !ca.Alive || ca.IsMerging {
			continue
		}
		for j := i + 1; j < len(p.Cells); j++ {
			cb := &p.Cells[j]
			if !cb.Alive || cb.IsMerging {
				continue
			}
			d := ca.Pos.Sub(cb.Pos).Length()
			minR := ca.Radius()
			if cb.Radius() < minR {
				minR = cb.Radius()
			}
			if d < minR*0.85 {
				// 合并到 ca（质量较大的保留，较小的合并过来）
				target, source := ca, cb
				if source.Mass > target.Mass {
					target, source = source, target
				}
				totalMass := target.Mass + source.Mass
				// 动量加权平均位置
				target.Pos = target.Pos.Mul(target.Mass/totalMass).Add(source.Pos.Mul(source.Mass / totalMass))
				target.Vel = target.Vel.Mul(target.Mass/totalMass).Add(source.Vel.Mul(source.Mass / totalMass))
				target.Mass = totalMass
				if target.Mass > Cfg.MaxMassPerCell {
					target.Mass = Cfg.MaxMassPerCell
				}
				source.Alive = false
			}
		}
	}
}

// applyInput 把虚拟游标转换为各 cell 的速度（鼠标跟随移动）
func (p *Player) applyInput(dt float64, worldW, worldH float64) {
	for i := range p.Cells {
		c := &p.Cells[i]
		if !c.Alive {
			continue
		}
		// 减少分裂冷却
		if c.SplitCooldown > 0 {
			c.SplitCooldown -= dt
		}

		dist := p.Cursor.Sub(c.Pos).Length()
		deadZone := c.Radius() * 0.5
		maxSpeed := Cfg.BaseSpeed / math.Sqrt(c.Mass)

		if dist < deadZone {
			c.Vel = Vec2{}
		} else {
			speedFactor := clamp((dist-deadZone)/(deadZone*3.0), 0, 1)
			// easeOutCubic 近似
			speedFactor = 1 - math.Pow(1-speedFactor, 3)
			dir := p.Cursor.Sub(c.Pos).Normalized()
			c.Vel = dir.Mul(maxSpeed * speedFactor)
		}
		c.Pos = c.Pos.Add(c.Vel.Mul(dt))
		// 边界钳制
		c.Pos.X = clamp(c.Pos.X, c.Radius(), worldW-c.Radius())
		c.Pos.Y = clamp(c.Pos.Y, c.Radius(), worldH-c.Radius())
	}

	if p.InvincibleTimer > 0 {
		p.InvincibleTimer -= dt
		if p.InvincibleTimer < 0 {
			p.InvincibleTimer = 0
		}
	}
}

// eatFood 检测 cell 吃食物
func (p *Player) eatFood(foods []Food) int {
	eaten := 0
	for i := range p.Cells {
		c := &p.Cells[i]
		if !c.Alive {
			continue
		}
		r := c.Radius()
		for j := range foods {
			if !foods[j].Alive {
				continue
			}
			d := foods[j].Pos.Sub(c.Pos).Length()
			if d < r {
				c.Mass += foods[j].Mass
				if c.Mass > Cfg.MaxMassPerCell {
					c.Mass = Cfg.MaxMassPerCell
				}
				foods[j].Alive = false
				eaten++
			}
		}
	}
	return eaten
}

// randFloat 返回 [min,max] 区间随机浮点
func randFloat(min, max float64) float64 {
	return min + globalRand.Float64()*(max-min)
}

func randInt(min, max int) int {
	return min + globalRand.Intn(max-min+1)
}

// Vec2FromAngle 创建指定角度的单位向量（角度弧度制）
func Vec2FromAngle(angle float64) Vec2 {
	return Vec2{math.Cos(angle), math.Sin(angle)}
}