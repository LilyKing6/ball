package world

import "math"

// CellObs cell 观察
type CellObs struct {
	X    float64 `json:"x"`
	Y    float64 `json:"y"`
	Mass float64 `json:"mass"`
}

// PlayerObs 玩家观察
type PlayerObs struct {
	ID      int       `json:"id"`
	Name    string    `json:"name"`
	Team    int       `json:"team"`
	Shield  int       `json:"shieldCount"`
	IsLocal bool      `json:"isLocal"`
	Cells   []CellObs `json:"cells"`
}

// FoodObs 食物观察
type FoodObs struct {
	X float64 `json:"x"`
	Y float64 `json:"y"`
}

// VirusObs 病毒观察
type VirusObs struct {
	X float64 `json:"x"`
	Y float64 `json:"y"`
}

// SporeObs 孢子观察
type SporeObs struct {
	X      float64 `json:"x"`
	Y      float64 `json:"y"`
	Mass   float64 `json:"mass"`
	Radius float64 `json:"radius"`
}

// BigBeanObs 大豆观察
type BigBeanObs struct {
	X    float64 `json:"x"`
	Y    float64 `json:"y"`
	Mass float64 `json:"mass"`
}

// Snapshot 世界快照（结构与 Qt 端 WorldSnapshot JSON 对齐）
type Snapshot struct {
	TickID           int         `json:"tickId"`
	GameTime         float64     `json:"gameTime"`
	WorldWidth       float64     `json:"worldWidth"`
	WorldHeight      float64     `json:"worldHeight"`
	GameMode         int         `json:"gameMode"`
	SafeZoneRadius   float64     `json:"safeZoneRadius"`
	SafeZoneCenterX  float64     `json:"safeZoneCenterX"`
	SafeZoneCenterY  float64     `json:"safeZoneCenterY"`
	ShrinkPhase      int         `json:"shrinkPhase"`
	TimeToNextShrink float64     `json:"timeToNextShrink"`
	Players          []PlayerObs `json:"players"`
	Foods            []FoodObs   `json:"foods"`
	Viruses          []VirusObs  `json:"viruses"`
	Spores           []SporeObs  `json:"spores"`
	BigBeans         []BigBeanObs `json:"bigBeans"`
}

// BuildSnapshot 构造当前帧快照
func (w *World) BuildSnapshot(tickID int) *Snapshot {
	s := &Snapshot{
		TickID:           tickID,
		GameTime:         w.GameTime,
		WorldWidth:       w.Width,
		WorldHeight:      w.Height,
		GameMode:         0, // free
		TimeToNextShrink: -1,
		Players:          make([]PlayerObs, 0, len(w.Players)),
		Foods:            make([]FoodObs, 0),
		Viruses:          make([]VirusObs, 0),
		Spores:           make([]SporeObs, 0),
		BigBeans:         make([]BigBeanObs, 0),
	}

	// 玩家
	for _, p := range w.Players {
		if p.Dead && len(p.Cells) == 0 {
			continue // 死亡且无存活 cell 的玩家不发送
		}
		po := PlayerObs{
			ID:     p.ID,
			Name:   p.Name,
			Team:   p.Team,
			Shield: p.Shield,
			Cells:  make([]CellObs, 0, len(p.Cells)),
		}
		for i := range p.Cells {
			if !p.Cells[i].Alive {
				continue
			}
			po.Cells = append(po.Cells, CellObs{
				X:    p.Cells[i].Pos.X,
				Y:    p.Cells[i].Pos.Y,
				Mass: p.Cells[i].Mass,
			})
		}
		s.Players = append(s.Players, po)
	}

	// 食物
	for i := range w.Foods {
		if !w.Foods[i].Alive {
			continue
		}
		s.Foods = append(s.Foods, FoodObs{
			X: w.Foods[i].Pos.X,
			Y: w.Foods[i].Pos.Y,
		})
	}

	// 病毒
	for i := range w.Viruses {
		if !w.Viruses[i].Alive {
			continue
		}
		s.Viruses = append(s.Viruses, VirusObs{
			X: w.Viruses[i].Pos.X,
			Y: w.Viruses[i].Pos.Y,
		})
	}

	// 孢子
	for i := range w.Spores {
		if !w.Spores[i].Alive {
			continue
		}
		s.Spores = append(s.Spores, SporeObs{
			X:      w.Spores[i].Pos.X,
			Y:      w.Spores[i].Pos.Y,
			Mass:   w.Spores[i].Mass,
			Radius: w.Spores[i].Radius(),
		})
	}

	// 大豆
	for i := range w.BigBeans {
		if !w.BigBeans[i].Alive {
			continue
		}
		s.BigBeans = append(s.BigBeans, BigBeanObs{
			X:    w.BigBeans[i].Pos.X,
			Y:    w.BigBeans[i].Pos.Y,
			Mass: w.BigBeans[i].Mass,
		})
	}

	return s
}

// unused import guard
var _ = math.Sqrt
