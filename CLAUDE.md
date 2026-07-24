# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project

**BallBattle** (球球大作战) — an agar.io-style "big ball eats small ball" competitive game built with Qt6 + OpenGL. Single-player vs AI (50 bots) with plans for multiplayer via WebSockets.

## Build & Run

```bash
# Configure (from project root)
cmake -B build -G "MinGW Makefiles" -DCMAKE_PREFIX_PATH=D:/Dev/Qt6/6.8.0/mingw_64

# Build
cmake --build build

# Run
./build/BallBattle.exe
```

The build uses **MinGW** (gcc via mingw64), not MSVC. Qt6 SDK is at `D:/Dev/Qt6/6.8.0/mingw_64`. The CMakeLists.txt copies `resources/` to the build directory automatically.

## Tech Stack

| Layer | Tech |
|-------|------|
| Framework | Qt 6 (Widgets, OpenGL, Sql, WebSockets, Charts) |
| Rendering | OpenGL 3.3 Core Profile (shader-based) + QPainter overlay |
| Build | CMake 3.20+, MinGW (gcc) |
| DB | SQLite via QtSql |
| Audio | QMediaPlayer + QSoundEffect |
| Language | C++17 |

## Architecture

### Entry Point & App Shell
- `src/main.cpp` → `GameApp` → creates `QApplication`, `GameEngine`, `MainWindow`
- `MainWindow` owns the GL widget (`QPainterGLWidget`), HUD overlay, and sub-windows (settings, records, rank) managed via `SubWindowManager` (QStack-based modal overlay)

### Two Rendering Paths
The project has **two parallel rendering systems**:
1. **QPainterGLWidget** (`src/renderer/QPainterGLWidget.h/cpp`) — the **active** rendering path. Uses QPainter on a QWidget for 2D drawing of balls, food, viruses, particles, minimap, and name labels. This is what MainWindow instantiates.
2. **GLRenderer** (`src/renderer/GLRenderer.h/cpp`) — OpenGL 3.3 Core shader-based renderer with dedicated sub-renderers (BallRenderer, FoodRenderer, VirusRenderer, SporeRenderer, GridRenderer). Uses `GLWidget` (QOpenGLWidget). **Not currently wired into MainWindow** — exists as an alternative/upgrade path.

### Game Loop (`GameEngine`)
- 60 FPS timer-driven loop in `QPainterGLWidget::gameLoop()`
- Fixed-timestep physics at 120 Hz (`accumulator += dt; while accumulator >= fixedDt: update(fixedDt)`)
- `GameEngine::update(float dt)` delegates to `World::update(dt)` which runs physics + AI

### Entity Hierarchy
- `Entity` (base: pos, vel, mass, alive) → `Food`, `Virus`, `Spore`, `Cell`
- `Player` owns a `QVector<Cell>` (up to 16 cells from splitting)
- `World` owns all entity collections: `QVector<Player>`, `QVector<Food>`, `QVector<Spore>`, `QVector<Virus>`, `QVector<AIController>`

### Physics (`PhysicsEngine`)
- Player movement: mouse-follow with dead zone, `speed = baseSpeed / sqrt(mass)`
- Collision: food absorption, player-vs-player devour (mass ratio > 1.1, overlap > 50%), virus triggering (mass > 110 causes forced split)
- Spatial hash grid (cell size 200) for O(n) collision broad-phase

### AI (`AIController` + `Behaviors.h`)
- State machine: Wander → SeekFood → ChaseSmaller → FleeBigger → SplitKill
- Three difficulties (Easy/Normal/Hard) with different perception radii, reaction delays, and split capability
- Each AI player gets one `AIController` instance stored in `World::m_aiControllers`

### Data Layer
- `DatabaseManager` — singleton, opens SQLite via QtSql, creates schema from `Schema::createTables()`
- `RecordManager` — singleton, saves/loads `GameRecord` and `PlayerStats` per player
- Schema: `player_profile`, `game_record`, `kill_detail`, `season_record`, `player_stats`, `unlock`

### Configuration
- `Config` singleton with hardcoded defaults; loads/saves `config.json` at runtime
- Key tunables: world size (6000×6000), food count (2000), virus count (20), max AI (50), initial mass (10), split min mass (36), merge cooldown (30s)

### Key Formulas
- Radius: `4 × sqrt(mass)`
- Speed: `300 / sqrt(mass)`
- Camera zoom: `1 / sqrt(mass / 20)`

## Key Directories

| Dir | Purpose |
|-----|---------|
| `src/engine/` | Game loop, state machine, World container |
| `src/entity/` | Entity base + Food, Virus, Spore, Player/Cell |
| `src/physics/` | Movement, collision detection |
| `src/ai/` | AI state machine + difficulty configs |
| `src/renderer/` | Both OpenGL and QPainter rendering paths |
| `src/particle/` | Particle system (burst, ring, trail, implode) |
| `src/skin/` | Ball skin system (Solid, Gradient, Striped, Dotted) |
| `src/ui/` | HUD, main menu, settings/records/rank sub-windows |
| `src/storage/` | SQLite schema + DatabaseManager singleton |
| `src/record/` | Game record save/load + player stats |
| `src/audio/` | SFX + BGM via QMediaPlayer |
| `src/util/` | Vec2 math, Config, Random |
| `resources/shaders/` | GLSL vertex/fragment shaders for OpenGL path |
| `resources/sounds/` | WAV sound effects (eat, split, death, etc.) |
| `docs/` | DESIGN.md (full architecture), TERMINOLOGY.md (game terms), P5_VISUAL_ENHANCEMENT.md |

## Empty/Planned Directories
`src/achievement/`, `src/input/`, `src/network/`, `src/ranking/`, `tests/` — directories exist but contain no source files. These are planned per DESIGN.md but not yet implemented.
