package world

import (
	"math"
)

// World 一个游戏世界（一个房间一个 World）
type World struct {
	Width    float64
	Height   float64
	Players  map[int]*Player
	Foods    []Food
	Spores   []Spore
	Viruses  []Virus
	BigBeans []BigBean
	TickID   int
	GameTime float64
}

// New 创建新世界
func New(width, height float64) *World {
	if width <= 0 {
		width = Cfg.WorldWidth
	}
	if height <= 0 {
		height = Cfg.WorldHeight
	}
	w := &World{
		Width:    width,
		Height:   height,
		Players:  make(map[int]*Player),
		Foods:    SpawnFoods(Cfg.FoodCount, width, height),
		Spores:   make([]Spore, 0),
		Viruses:  spawnViruses(int(Cfg.VirusCount), width, height),
		BigBeans: spawnBigBeans(int(Cfg.BigBeanCount), width, height),
	}
	return w
}

// spawnViruses 在世界中生成病毒
func spawnViruses(count int, worldW, worldH float64) []Virus {
	viruses := make([]Virus, count)
	for i := range viruses {
		viruses[i] = NewVirus(Vec2{
			X: randFloat(200, worldW-200),
			Y: randFloat(200, worldH-200),
		})
	}
	return viruses
}

// spawnBigBeans 在世界中生成大豆
func spawnBigBeans(count int, worldW, worldH float64) []BigBean {
	bbs := make([]BigBean, count)
	for i := range bbs {
		bbs[i] = NewBigBean(
			Vec2{X: randFloat(100, worldW-100), Y: randFloat(100, worldH-100)},
			Cfg.BigBeanMinMass, Cfg.BigBeanMaxMass,
		)
	}
	return bbs
}

// AddPlayer 添加玩家
func (w *World) AddPlayer(id int, name string) *Player {
	p := NewPlayer(id, name, w.Width, w.Height)
	w.Players[id] = p
	return p
}

// RemovePlayer 移除玩家
func (w *World) RemovePlayer(id int) {
	delete(w.Players, id)
}

// SetCursor 应用玩家虚拟游标
func (w *World) SetCursor(playerID int, cursor Vec2) {
	if p, ok := w.Players[playerID]; ok {
		p.Cursor = cursor
	}
}

// SetWantSplit 设置玩家分裂标志
func (w *World) SetWantSplit(playerID int) {
	if p, ok := w.Players[playerID]; ok {
		p.WantSplit = true
	}
}

// SetWantEject 设置玩家吐球标志
func (w *World) SetWantEject(playerID int) {
	if p, ok := w.Players[playerID]; ok {
		p.WantEject = true
	}
}

// Step 推进一个物理步长
func (w *World) Step(dt float64) {
	w.TickID++
	w.GameTime += dt

	// 1. 玩家移动 + 减少冷却
	for _, p := range w.Players {
		p.applyInput(dt, w.Width, w.Height)
	}

	// 2. 处理 WantSplit
	for _, p := range w.Players {
		if p.WantSplit && !p.Dead && p.canSplit() {
			newCells := p.split(w.Width, w.Height)
			p.Cells = append(p.Cells, newCells...)
		}
		p.WantSplit = false
	}

	// 3. 处理 WantEject
	for _, p := range w.Players {
		if p.WantEject && !p.Dead && p.TotalMass() > Cfg.EjectMass+10 {
			spores := p.eject(w.Width, w.Height)
			space := Cfg.MaxSpores - len(w.Spores)
			if space > len(spores) {
				space = len(spores)
			}
			if space > 0 {
				w.Spores = append(w.Spores, spores[:space]...)
			}
		}
		p.WantEject = false
	}

	// 4. 玩家吃食物
	for _, p := range w.Players {
		p.eatFood(w.Foods)
	}

	// 5. 玩家间吞噬
	playersList := make([]*Player, 0, len(w.Players))
	for _, p := range w.Players {
		if !p.Dead {
			playersList = append(playersList, p)
		}
	}
	for i := 0; i < len(playersList); i++ {
		for j := i + 1; j < len(playersList); j++ {
			devour(playersList[i], playersList[j])
		}
	}

	// 6. 玩家吃孢子
	for _, p := range w.Players {
		if p.Dead {
			continue
		}
		for ci := range p.Cells {
			c := &p.Cells[ci]
			if !c.Alive {
				continue
			}
			r := c.Radius()
			for si := range w.Spores {
				s := &w.Spores[si]
				if !s.Alive {
					continue
				}
				if s.ImmunityTimer > 0 && s.OwnerID == p.ID {
					continue
				}
				if s.Pos.Sub(c.Pos).Length() < r {
					c.Mass += s.Mass
					if c.Mass > Cfg.MaxMassPerCell {
						c.Mass = Cfg.MaxMassPerCell
					}
					s.Alive = false
				}
			}
		}
	}

	// 6b. 清理死亡孢子
	aliveSpores := make([]Spore, 0, len(w.Spores))
	for i := range w.Spores {
		if w.Spores[i].Alive {
			aliveSpores = append(aliveSpores, w.Spores[i])
		}
	}
	w.Spores = aliveSpores

	// 7. 病毒碰撞 → 玩家分裂成碎片
	for vi := range w.Viruses {
		v := &w.Viruses[vi]
		if !v.Alive {
			continue
		}
		for _, p := range w.Players {
			if p.Dead {
				continue
			}
			for ci := range p.Cells {
				c := &p.Cells[ci]
				if !c.Alive {
					continue
				}
				dist := c.Pos.Sub(v.Pos).Length()
				if dist < c.Radius()+v.Radius() {
					if c.Mass > v.Mass*Cfg.MassRatioForEat {
						virusMass := v.Mass
						c.Mass += virusMass
						if c.Mass > Cfg.MaxMassPerCell {
							c.Mass = Cfg.MaxMassPerCell
						}
						// 分裂碎片
						fragmentCount := Cfg.VirusFragmentCount
						maxNewCells := Cfg.MaxCellsPerPlayer - len(p.Cells)
						if maxNewCells <= 0 {
							v.Alive = false
							continue
						}
						if fragmentCount > maxNewCells+1 {
							fragmentCount = maxNewCells + 1
						}
						totalMass := c.Mass
						perFragmentMass := totalMass / float64(fragmentCount)
						c.Mass = perFragmentMass
						c.IsMerging = true
						c.MergeTimer = Cfg.MergeCooldown
						for fi := 1; fi < fragmentCount; fi++ {
							angle := float64(fi) * 2.0 * math.Pi / float64(fragmentCount)
							dir := Vec2FromAngle(angle)
							nc := Cell{
								Pos:           c.Pos.Add(dir.Mul(c.Radius() * 3)),
								Vel:           dir.Mul(Cfg.VirusFragmentVelocity),
								Mass:          perFragmentMass,
								Alive:         true,
								SplitCooldown: Cfg.SplitCooldown,
								IsMerging:     true,
								MergeTimer:    Cfg.MergeCooldown,
							}
							p.Cells = append(p.Cells, nc)
						}
						v.Alive = false
					}
				}
			}
		}
	}

	// 8. 大豆碰撞 → 加质量
	for bi := range w.BigBeans {
		bb := &w.BigBeans[bi]
		if !bb.Alive {
			continue
		}
		for _, p := range w.Players {
			if p.Dead {
				continue
			}
			for ci := range p.Cells {
				c := &p.Cells[ci]
				if !c.Alive {
					continue
				}
				dist := c.Pos.Sub(bb.Pos).Length()
				if dist < c.Radius() {
					c.Mass += bb.Mass
					if c.Mass > Cfg.MaxMassPerCell {
						c.Mass = Cfg.MaxMassPerCell
					}
					bb.Alive = false
				}
			}
		}
	}

	// 8b. 重新生成被吃掉的大豆
	for i := range w.BigBeans {
		if !w.BigBeans[i].Alive {
			w.BigBeans[i] = NewBigBean(
				Vec2{X: randFloat(100, w.Width-100), Y: randFloat(100, w.Height-100)},
				Cfg.BigBeanMinMass, Cfg.BigBeanMaxMass,
			)
		}
	}

	// 9. 孢子更新
	for i := range w.Spores {
		w.Spores[i].Update(dt, w.Width, w.Height)
	}

	// 10. 病毒复活
	for i := range w.Viruses {
		if !w.Viruses[i].Alive {
			w.Viruses[i].RespawnTimer -= dt
			if w.Viruses[i].RespawnTimer <= 0 {
				w.Viruses[i] = NewVirus(Vec2{
					X: randFloat(200, w.Width-200),
					Y: randFloat(200, w.Height-200),
				})
			}
		}
	}

	// 11. 食物 respawn
	RespawnFoods(w.Foods, w.Width, w.Height)

	// 12. 融合计时
	for _, p := range w.Players {
		if !p.Dead {
			p.updateMerge(dt)
		}
	}

	// 13. 死亡检测
	for _, p := range w.Players {
		if !p.Dead && p.IsDead() {
			p.Dead = true
			// AI 玩家自动复活
			if p.IsAI {
				p.RespawnTimer = Cfg.RespawnDelay
			}
		}
		// AI 复活
		if p.IsAI && p.Dead && p.RespawnTimer > 0 {
			p.RespawnTimer -= dt
			if p.RespawnTimer <= 0 {
				p.Cells = []Cell{{
					Pos:   Vec2{randFloat(100, w.Width-100), randFloat(100, w.Height-100)},
					Mass:  Cfg.InitialMass,
					Alive: true,
				}}
				p.Dead = false
				p.DeathNotified = false
				p.InvincibleTimer = 3.0
			}
		}
	}
}

// devour a 和 b 间的吞噬判定
func devour(a, b *Player) {
	if a.Dead || b.Dead {
		return
	}
	if a.Team != 0 && a.Team == b.Team {
		return
	}

	// a 吃 b
	for i := range a.Cells {
		ca := &a.Cells[i]
		if !ca.Alive {
			continue
		}
		for j := range b.Cells {
			cb := &b.Cells[j]
			if !cb.Alive {
				continue
			}
			// 出生无敌
			if b.InvincibleTimer > 0 {
				continue
			}
			if ca.Mass > cb.Mass*Cfg.MassRatioForEat {
				d := ca.Pos.Sub(cb.Pos).Length()
				if d < ca.Radius()-cb.Radius()*Cfg.OverlapRatioForEat {
					ca.Mass += cb.Mass
					if ca.Mass > Cfg.MaxMassPerCell {
						ca.Mass = Cfg.MaxMassPerCell
					}
					cb.Alive = false
				}
			}
		}
	}

	// b 吃 a
	for j := range b.Cells {
		cb := &b.Cells[j]
		if !cb.Alive {
			continue
		}
		for i := range a.Cells {
			ca := &a.Cells[i]
			if !ca.Alive {
				continue
			}
			if a.InvincibleTimer > 0 {
				continue
			}
			if cb.Mass > ca.Mass*Cfg.MassRatioForEat {
				d := cb.Pos.Sub(ca.Pos).Length()
				if d < cb.Radius()-ca.Radius()*Cfg.OverlapRatioForEat {
					cb.Mass += ca.Mass
					if cb.Mass > Cfg.MaxMassPerCell {
						cb.Mass = Cfg.MaxMassPerCell
					}
					ca.Alive = false
				}
			}
		}
	}
}

// 避免 unused import
var _ = math.Sqrt
