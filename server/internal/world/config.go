package world

// Cfg 全局游戏常量。Run() 中可在启动时被覆写
var Cfg = struct {
	WorldWidth         float64
	WorldHeight        float64
	InitialMass        float64
	BaseSpeed          float64
	RadiusConstant     float64
	FoodCount          int
	FoodRadius         float64
	FoodMass           float64
	MaxMassPerCell     float64
	MassRatioForEat    float64
	OverlapRatioForEat float64
	FixedDt            float64

	// Split / Eject / Merge
	SplitMinMass         float64
	SplitVelocity        float64
	EjectMass            float64
	EjectVelocity        float64
	MaxCellsPerPlayer    int
	MergeCooldown        float64
	SplitCooldown        float64

	// Spore
	SporeSpeed         float64
	SporeImmunityTime  float64
	SporeDecayRate     float64
	SporeVelocityDecay float64
	MaxSpores          int

	// Virus
	VirusCount            int
	VirusMass             float64
	VirusSplitThreshold   float64
	VirusFragmentCount    int
	VirusFragmentVelocity float64
	VirusRespawnTime      float64

	// BigBean
	BigBeanCount   int
	BigBeanMinMass float64
	BigBeanMaxMass float64

	// Game
	RespawnDelay float64

	// View culling
	ViewCullRadius float64 // 视野裁剪半径（世界坐标），<=0 表示不裁剪
}{
	WorldWidth:         3000,
	WorldHeight:        3000,
	InitialMass:        10,
	BaseSpeed:          300,
	RadiusConstant:     6,
	FoodCount:          500,
	FoodRadius:         3,
	FoodMass:           1,
	MaxMassPerCell:     40000,
	MassRatioForEat:    1.1,
	OverlapRatioForEat: 0.5,
	FixedDt:            1.0 / 60.0,

	// Split / Eject / Merge
	SplitMinMass:      36,
	SplitVelocity:     800,
	EjectMass:         14,
	EjectVelocity:     400,
	MaxCellsPerPlayer: 16,
	MergeCooldown:     30.0,
	SplitCooldown:     0.5,

	// Spore
	SporeSpeed:         400,
	SporeImmunityTime:  0.3,
	SporeDecayRate:     0.3,
	SporeVelocityDecay: 0.98,
	MaxSpores:          200,

	// Virus
	VirusCount:            30,
	VirusMass:             100,
	VirusSplitThreshold:   200,
	VirusFragmentCount:    9,
	VirusFragmentVelocity: 500,
	VirusRespawnTime:      10,

	// BigBean
	BigBeanCount:   15,
	BigBeanMinMass: 100,
	BigBeanMaxMass: 500,

	// Game
	RespawnDelay: 3.0,

	// View culling
	ViewCullRadius: 1500.0,
}
