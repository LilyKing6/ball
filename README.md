# BallBattle（球球大作战）

一个 Agar.io 风格的“大球吃小球”竞技游戏。Qt6 + OpenGL 客户端，Go + WebSocket 服务端，支持单机对战 AI 与局域网/在线多人联机。

![Platform](https://img.shields.io/badge/Platform-Windows-blue)
![Qt](https://img.shields.io/badge/Qt-6.8+-green)
![Go](https://img.shields.io/badge/Go-1.26+-cyan)
![License](https://img.shields.io/badge/License-MIT-yellow)

---

## 特性

- **经典玩法**：移动、吃食物、吞噬对手、分裂、吐球、病毒互动
- **多种模式**：自由模式、极速模式、团战模式、大逃杀模式
- **AI 对手**：最多 50 个 Bot，三种难度
- **多人联机**：Go 服务端 + Qt WebSocket 客户端，支持多客户端同房间对战
- **进度系统**：段位（ELO）、赛季、成就、游戏记录与统计
- **视觉**：OpenGL 3.3 Core 渲染、粒子效果、小地图、皮肤系统、动态主菜单
- **音效**：QMediaPlayer + QSoundEffect 背景音乐与音效

---

## 技术栈

| 层级 | 技术 |
|------|------|
| 客户端框架 | Qt 6 (Widgets, OpenGL, Sql, WebSockets, Charts) |
| 客户端渲染 | OpenGL 3.3 Core + QPainter 覆盖层 |
| 服务端 | Go 1.26 + gorilla/websocket |
| 构建 | CMake 3.20+, MinGW (gcc) |
| 数据库 | SQLite (QtSql) |
| 音频 | QMediaPlayer + QSoundEffect |
| 语言 | C++17 / Go |

---

## 构建与运行

### 环境要求

- Windows 10/11
- Qt 6.8+ SDK（MinGW 版本）
- MinGW-w64 (gcc)
- CMake 3.20+
- Go 1.26+（如需运行服务端）

### 客户端

```bash
# 配置（在项目根目录执行）
cmake -B build -G "MinGW Makefiles" -DCMAKE_PREFIX_PATH=D:/Dev/Qt6/6.8.0/mingw_64

# 构建
cmake --build build --parallel

# 运行
./build/BallBattle.exe
```

### 服务端（联机模式）

```bash
cd server
go build -buildvcs=false -o ballbattle-server.exe .
./ballbattle-server.exe -addr :8765
```

启动客户端后：主菜单 → 模式选择 → 联网对战 → 输入 `127.0.0.1:8765` 和玩家名。

---

## 项目结构

```
ball/
├── CMakeLists.txt          # 客户端构建配置
├── README.md               # 本文件
├── src/                    # Qt 客户端源码
│   ├── app/                # GameApp、MainWindow
│   ├── engine/             # GameEngine、World、GameState、WorldSnapshot
│   ├── entity/             # Player/Cell、Food、Virus、Spore、BigBean
│   ├── physics/            # PhysicsEngine
│   ├── ai/                 # AIController
│   ├── renderer/           # GLWidget、GLRenderer、QPainterGLWidget
│   ├── ui/                 # MainMenu、HUD、子窗口
│   ├── network/            # NetworkClient、ExternalAgentServer
│   ├── particle/           # 粒子系统
│   ├── skin/               # 皮肤系统
│   ├── storage/            # SQLite DatabaseManager
│   ├── record/             # 游戏记录与统计
│   ├── ranking/            # RankSystem、SeasonManager
│   ├── achievement/        # AchievementManager
│   ├── audio/              # AudioManager
│   └── util/               # Config、Math、Random
├── server/                 # Go 服务端
│   ├── main.go
│   └── internal/           # proto、server、world
├── resources/              # 着色器、音效、图片资源
├── docs/                   # 设计文档
└── tools/                  # 测试脚本
```

---

## 文档

- [`docs/DESIGN.md`](docs/DESIGN.md) — 完整架构设计文档
- [`docs/TERMINOLOGY.md`](docs/TERMINOLOGY.md) — 游戏术语规范
- [`docs/M1_NETWORK_MIGRATION.md`](docs/M1_NETWORK_MIGRATION.md) — 网络迁移文档
- [`docs/P5_VISUAL_ENHANCEMENT.md`](docs/P5_VISUAL_ENHANCEMENT.md) — 视觉增强文档

---

## 贡献

欢迎提交 Issue 和 PR。 major 改动请先阅读 `docs/DESIGN.md`。

---

## 许可

MIT License
