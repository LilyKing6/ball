package world

import "math/rand"

// Food 食物
type Food struct {
	Pos   Vec2
	Mass  float64
	Alive bool
}

var globalRand = rand.New(rand.NewSource(1))

// SpawnFoods 生成初始食物
func SpawnFoods(count int, worldW, worldH float64) []Food {
	foods := make([]Food, count)
	for i := range foods {
		foods[i] = Food{
			Pos:   Vec2{randFloat(0, worldW), randFloat(0, worldH)},
			Mass:  Cfg.FoodMass,
			Alive: true,
		}
	}
	return foods
}

// RespawnFoods 把死亡的食物重新放置
func RespawnFoods(foods []Food, worldW, worldH float64) {
	for i := range foods {
		if !foods[i].Alive {
			foods[i].Pos = Vec2{randFloat(0, worldW), randFloat(0, worldH)}
			foods[i].Alive = true
		}
	}
}
