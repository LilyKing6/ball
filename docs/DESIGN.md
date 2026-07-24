# 球球大作战 - 设计文档

> Qt6 + OpenGL 增强版 | 支持单机/联机 | 段位 + 记录 + 成就系统

---

## 目录

1. [项目概述](#1-项目概述)
2. [项目架构](#2-项目架构)
3. [核心系统设计](#3-核心系统设计)
4. [游戏机制](#4-游戏机制)
5. [渲染系统](#5-渲染系统)
6. [AI系统](#6-ai系统)
7. [段位系统](#7-段位系统)
8. [游戏记录系统](#8-游戏记录系统)
9. [成就系统](#9-成就系统)
10. [输入系统](#10-输入系统)
11. [网络模块](#11-网络模块)
12. [界面设计](#12-界面设计)
13. [视觉规范](#13-视觉规范)
14. [分阶段实施计划](#14-分阶段实施计划)

---

## 1. 项目概述

### 技术栈

| 层级 | 技术 |
|------|------|
| 框架 | Qt 6 (C++17) |
| 渲染 | OpenGL 3.3 Core Profile |
| 构建 | CMake 3.20+ |
| 数据库 | SQLite3 (QtSql模块) |
| 网络 | QWebSocket |
| 图表 | Qt Charts |
| 音频 | QMediaPlayer + QSoundEffect |

### 关键技术参数

| 参数 | 值 |
|------|-----|
| 世界大小 | 6000 x 6000 |
| 初始质量 | 10 |
| 食物数量 | ~2000 |
| 病毒数量 | ~20 |
| 最大AI数 | 50 |
| 最大Cell数/玩家 | 16 |
| 分裂最小质量 | 36 |
| 合并等待时间 | 30秒 |
| 帧率 | 60FPS渲染 / 120Hz物理 |

---

## 2. 项目架构

### 目录结构

```
ball/
├── CMakeLists.txt
├── docs/
│   └── DESIGN.md
├── src/
│   ├── main.cpp
│   ├── app/
│   │   ├── GameApp.h/cpp
│   │   └── MainWindow.h/cpp
│   ├── engine/
│   │   ├── GameEngine.h/cpp
│   │   ├── GameState.h/cpp
│   │   └── World.h/cpp
│   ├── entity/
│   │   ├── Entity.h/cpp
│   │   ├── Cell.h/cpp
│   │   ├── Player.h/cpp
│   │   ├── Food.h/cpp
│   │   ├── Virus.h/cpp
│   │   └── Spawner.h/cpp
│   ├── physics/
│   │   ├── PhysicsEngine.h/cpp
│   │   └── Collision.h/cpp
│   ├── ai/
│   │   ├── AIController.h/cpp
│   │   └── Behaviors.h/cpp
│   ├── renderer/
│   │   ├── GLWidget.h/cpp
│   │   ├── GLRenderer.h/cpp
│   │   ├── Camera.h/cpp
│   │   ├── ShaderManager.h/cpp
│   │   ├── GridRenderer.h/cpp
│   │   ├── BallRenderer.h/cpp
│   │   ├── FoodRenderer.h/cpp
│   │   ├── VirusRenderer.h/cpp
│   │   ├── ParticleRenderer.h/cpp
│   │   ├── MinimapRenderer.h/cpp
│   │   └── TextRenderer.h/cpp
│   ├── ui/
│   │   ├── HUDOverlay.h/cpp
│   │   ├── LeaderboardWidget.h/cpp
│   │   ├── MainMenu.h/cpp
│   │   ├── SkinSelector.h/cpp
│   │   ├── SettingsDialog.h/cpp
│   │   ├── ChatWidget.h/cpp
│   │   ├── RankBadgeWidget.h/cpp
│   │   ├── RankProgressWidget.h/cpp
│   │   ├── RecordListWidget.h/cpp
│   │   ├── RecordDetailWidget.h/cpp
│   │   ├── StatsDashboard.h/cpp
│   │   ├── SeasonWidget.h/cpp
│   │   ├── AchievementWidget.h/cpp
│   │   ├── GameOverScreen.h/cpp
│   │   └── LobbyWidget.h/cpp
│   ├── network/
│   │   ├── NetworkManager.h/cpp
│   │   ├── GameServer.h/cpp
│   │   ├── GameClient.h/cpp
│   │   └── Protocol.h/cpp
│   ├── audio/
│   │   └── AudioManager.h/cpp
│   ├── particle/
│   │   ├── ParticleSystem.h/cpp
│   │   └── Particle.h/cpp
│   ├── skin/
│   │   └── SkinManager.h/cpp
│   ├── ranking/
│   │   ├── RankSystem.h/cpp
│   │   ├── RankTier.h/cpp
│   │   ├── SeasonManager.h/cpp
│   │   └── RankReward.h/cpp
│   ├── record/
│   │   ├── RecordManager.h/cpp
│   │   ├── GameRecord.h/cpp
│   │   ├── PlayerStats.h/cpp
│   │   ├── RecordDatabase.h/cpp
│   │   ├── RecordQuery.h/cpp
│   │   └── StatsCalculator.h/cpp
│   ├── storage/
│   │   ├── DatabaseManager.h/cpp
│   │   ├── Migration.h/cpp
│   │   └── Schema.h
│   ├── achievement/
│   │   ├── AchievementManager.h/cpp
│   │   └── AchievementDef.h
│   ├── input/
│   │   ├── InputManager.h/cpp
│   │   ├── KeyBinding.h/cpp
│   │   ├── KeyBindingConfig.h/cpp
│   │   └── InputContext.h/cpp
│   └── util/
│       ├── Math.h/cpp
│       ├── Random.h/cpp
│       ├── Config.h/cpp
│       └── SpatialHash.h/cpp
├── resources/
│   ├── shaders/
│   │   ├── ball.vert / ball.frag
│   │   ├── grid.vert / grid.frag
│   │   ├── food.vert / food.frag
│   │   ├── virus.vert / virus.frag
│   │   ├── particle.vert / particle.frag
│   │   └── text.vert / text.frag
│   ├── textures/
│   ├── sounds/
│   ├── skins/
│   ├── db/
│   │   └── v1_init.sql
│   └── achievements/
│       └── definitions.json
└── tests/
```

---

## 3. 核心系统设计

### 3.1 游戏循环 (`GameEngine`)

定时器驱动，固定时间步物理更新 + 可变渲染：

```
QTimer(16ms ≈ 60fps)
    |
    v
+-------------+
| 输入处理      | <- InputManager 事件队列
+------+------+
       v
+-------------+     accumulator += deltaTime
| 物理更新      |     while (accumulator >= fixedDt)
| (120Hz固定)  |         update(fixedDt=1/120)
+------+------+         accumulator -= fixedDt
       v
+-------------+
| 渲染 (60Hz)  | <- VSync / QTimer间隔控制
+------+------+
       v
+-------------+
| 帧结束       | <- FPS统计、性能监控
+-------------+
```

### 3.2 游戏状态机 (`GameState`)

```
+----------+   开始游戏   +----------+
|  Menu    |------------->| Playing  |
+----------+              +----+-----+
     ^                        |   |
     |                 暂停Esc|   |死亡/退出
     |                        v   v
     |                   +----+---+
     |                   |  Paused  |
     |                   +----------+
     |                        |继续
     |                        v
     |                  +----------+
     +-----------------| GameOver |
         返回主菜单     +----------+
```

### 3.3 世界 (`World`)

- 正方形边界 6000x6000
- 坐标系：左上角(0,0)，右下角(6000,6000)
- 边界碰撞：球体不可超出，靠近边界时速度衰减推回
- 空间分区：SpatialHash 将世界划分为网格单元，加速碰撞检测

### 3.4 空间哈希 (`SpatialHash`)

```
cellSize = 200 (可配置)
hash(x, y) = (floor(x/cellSize), floor(y/cellSize))

插入: 将实体加入对应格子
查询: 查目标实体所在格子 + 相邻8格 -> 候选碰撞集
复杂度: O(n) 平均，避免 O(n^2) 全量检测
```

### 3.5 相机系统 (`Camera`)

- 平滑跟随玩家质心：`pos = lerp(pos, targetPos, 0.1)`
- 缩放与总质量成反比：`zoom = baseZoom / sqrt(totalMass / initialMass)`
- 分裂时自动扩展视野包含所有Cell
- 鼠标滚轮手动微调，5秒无操作恢复自动

---

## 4. 游戏��制

### 4.1 移动

鼠标跟随方向 + WASD辅助（可配置，见第10章）：

```
鼠标位置: M (世界坐标)
球体位置: P (世界坐标)
方向向量: D = normalize(M - P)
距离:     dist = length(M - P)

死区半径: deadZone = 球半径 * deadZoneFactor (默认0.5)
最大速度: maxSpeed = baseSpeed / sqrt(mass)

if (dist < deadZone) {
    velocity = 0
} else {
    speedFactor = min(1.0, (dist - deadZone) / (deadZone * 3))
    velocity = D * maxSpeed * easeOutCubic(speedFactor)
}

// easeOutCubic(t) = 1 - (1-t)^3
```

### 4.2 质量与半径

```
radius = k * sqrt(mass)    // k为常数，默认 k=4
初始质量=10 -> radius约12.6
质量100   -> radius=40
质量1000  -> radius约126.5
```

### 4.3 吃食物/吃球

- 碰撞条件：大球中心进入小球范围内（重叠度>50%）
- 质量守恒：大球质量 += 小球质量
- 吃食物：+1 质量
- 吃球：吸收对方全部质量

### 4.4 分裂 (`Split`)

- 按键：Space（默认，可配置）
- 方向：沿鼠标方向射出
- 条件：当前Cell质量 >= 36
- 分裂后：原Cell质量减半，新Cell获得另一半
- 射出速度：初始高速，逐渐减速回归正常
- 最大Cell数：16个
- 冷却时间：0.5秒

### 4.5 吐球 (`Eject`)

- 按键：E（默认，可配置）
- 方向：沿鼠标方向射出
- 每次吐出质量14的小球
- 射出速度固定，小球沿直线飞行
- 按住可连续吐，每100ms一颗
- 双击E快速吐出3颗

### 4.6 合并

- 分裂后30秒才能合并（冷却计时器）
- 合并条件：两Cell中心距离 < 两半径之和的50%
- 合并动画：小球滑入大球
- 无其他Cell阻挡路径

### 4.7 病毒 (`Virus`)

- 绿色大尖刺球，固定位置
- 玩家Cell质量 > 病毒质量时触碰 -> 强制分裂成多个小Cell
- 小于病毒质量的Cell穿过病毒不受影响
- 病毒被触发后消失，一段时间后在新位置重生
- 可用吐球将食物射入病毒使其分裂->向最近玩家方向弹射

---

## 5. 渲染系统

### 5.1 渲染管线总览

```
GLWidget::paintGL()
    |
    v
GLRenderer::render()
    |
    +-- 清屏 (深色背景)
    |
    +-- 设置相机矩阵 (projection * view)
    |
    +-- GridRenderer::render()       // 背景网格 + 世界边界
    |
    +-- FoodRenderer::render()       // Instanced批量绘制食物
    |
    +-- VirusRenderer::render()      // 病毒尖刺球
    |
    +-- BallRenderer::render()       // 所有玩家球体(含AI)
    |   +-- 渐变填充
    |   +-- 描边
    |   +-- 名字标签
    |
    +-- ParticleRenderer::render()   // 粒子效果
    |
    +-- TextRenderer::render()       // 游戏内文字(质量数字等)
    |
    +-- MinimapRenderer::render()    // 右下小地图(独立投影)
```

### 5.2 球体着色器 (`ball.vert / ball.frag`)

顶点着色器：标准MVP变换，传递局部坐标用于SDF计算。

片段着色器核心逻辑：

```glsl
// SDF圆形 + 平滑边缘
float dist = length(localPos);
float radius = 1.0;
float edge = smoothstep(radius, radius - 0.02, dist);

// 渐变填充：中心亮，边缘暗
vec3 gradient = mix(lightColor, darkColor, dist / radius);

// 描边：边缘2px环
float outlineWidth = 0.03;
float outline = smoothstep(radius, radius - outlineWidth, dist)
              - smoothstep(radius - outlineWidth, radius - outlineWidth * 2, dist);

vec3 finalColor = mix(outlineColor, gradient, 1.0 - outline);
fragColor = vec4(finalColor, edge);
```

### 5.3 食物着色器 (`food.vert / food.frag`)

Instanced渲染，一次DrawCall绘制全部食物：

```glsl
// 顶点着色器
layout(location = 0) in vec2 position;      // 单位圆顶点
layout(location = 1) in vec2 instancePos;   // 实例位置
layout(location = 2) in vec4 instanceColor; // 实例颜色
layout(location = 3) in float instanceSize; // 实例大小

// 片段着色器 - 简单SDF圆 + 纯色 + 轻微发光
```

### 5.4 网格着色器 (`grid.vert / grid.frag`)

无限网格效果，跟随相机平移和缩放：

```glsl
// 基于世界坐标计算网格线
vec2 grid = abs(fract(worldPos / gridSize - 0.5) - 0.5);
float line = min(grid.x, grid.y);
float alpha = 1.0 - smoothstep(0.0, lineWidth, line);

// 世界边界高亮
float border = step(worldPos.x, 0.0) + step(6000.0, worldPos.x)
             + step(worldPos.y, 0.0) + step(6000.0, worldPos.y);
```

### 5.5 病毒着色器 (`virus.vert / virus.frag`)

SDF尖刺球效果：

```glsl
// 极坐标 + 噪声扰动产生尖刺
float angle = atan(localPos.y, localPos.x);
float spike = sin(angle * 12.0) * 0.15;  // 12个尖刺
float dist = length(localPos);
float shape = 1.0 - smoothstep(1.0 + spike - 0.02, 1.0 + spike, dist);
```

### 5.6 粒子着色器 (`particle.vert / particle.frag`)

Point Sprite + Alpha/Additive混合：

```glsl
// 圆形衰减 + 颜色 + 透明度生命周期衰减
float alpha = life * (1.0 - length(gl_PointCoord - 0.5) * 2.0);
```

### 5.7 文字着色器 (`text.vert / text.frag`)

SDF文字渲染：

```glsl
// 采样距离场，smoothstep抗锯齿
float dist = texture(sdfAtlas, texCoord).r;
float alpha = smoothstep(0.5 - smoothing, 0.5 + smoothing, dist);
```

### 5.8 ShaderManager

- 启动时编译链接所有着色器程序
- 缓存uniform location
- 热重载支持（开发模式监听文件变化）

---

## 6. AI系统

### 6.1 AI控制器状态机 (`AIController`)

```
         +----------+
    +----|  Wander   |----+
    |    +----------+    |
    | 随机漫游发现目标    | 无威胁且有食物
    |                    v
    |             +----------+
    |             | SeekFood  |----+
    |             +----------+    |发现可吃的小球
    |                             v
    |                      +-----------+
    |                      |ChaseSmaller|--+
    |                      +-----------+  |遇到更大球
    |                                     v
    |                              +----------+
    |  威胁消失                    |FleeBigger |
    |<-----------------------------+----------+
    |                                     |
    |  高AI + 可分裂击杀                  |条件满足
    |                                     v
    |                              +-----------+
    |                              | SplitKill  |
    |                              +-----------+
    +--------------------------------------+
```

### 6.2 行为决策

```
感知半径 = 800 + mass * 2

每帧评估:
1. 扫描感知半径内所有实体
2. 计算威胁值: 比自己大的球 -> 威胁距离/质量比
3. 计算收益值: 食物/小球 -> 质量收益/距离比
4. 选择最优行为

优先级:
  FleeBigger > SplitKill > ChaseSmaller > SeekFood > Wander
```

### 6.3 AI难度

| 难度 | 感知半径 | 反应延迟 | 分裂能力 | 决策质量 |
|------|---------|---------|---------|---------|
| 简单 | 600 | 500ms | 无 | 随机 |
| 普通 | 800 | 200ms | 偶尔 | 次优 |
| 困难 | 1200 | 50ms | 熟练 | 近最优 |

---

## 7. 段位系统

### 7.1 段位等级 (`RankTier`)

| 段位 | 英文 | 色值 | ELO范围 | 子级 |
|------|------|------|---------|------|
| 青铜 | Bronze | #CD7F32 | 0-799 | I / II / III |
| 白银 | Silver | #C0C0C0 | 800-1199 | I / II / III |
| 黄金 | Gold | #FFD700 | 1200-1599 | I / II / III |
| 铂金 | Platinum | #E5E4E2 | 1600-1999 | I / II / III |
| 钻石 | Diamond | #B9F2FF | 2000-2499 | I / II / III |
| 大师 | Master | #FF4500 | 2500-2999 | I / II |
| 王者 | Champion | #FF0000 | 3000+ | 唯一 |

子级划分（以青铜为例）：
- 青铜 III: 0-266
- 青铜 II: 267-533
- 青铜 I: 534-799

### 7.2 ELO计算 (`RankSystem`)

```
K因子:
  ELO < 2400 -> K = 32
  ELO >= 2400 -> K = 24

预期胜率:
  E_a = 1 / (1 + 10^((E_b - E_a) / 400))

每局得分:
  score = killScore + survivalScore + rankScore

  killScore:
    每击杀 = K * 0.1 * (1 - E_killer/E_victim)
    击杀高ELO玩家额外奖励

  survivalScore:
    存活每分钟 = +2

  rankScore:
    排名前3 = +K * (1 - rank/totalPlayers)
    排名后50% = -K * 0.05 * (rank/totalPlayers)

ELO变化:
  newElo = oldElo + totalScore
  最低ELO = 0 (不降为负)

保级机制:
  刚升入新段位 -> 3局保护期(不会降回上一段位)
```

### 7.3 赛季管理 (`SeasonManager`)

- 每赛季30天
- 赛季结束软重置：`newElo = 1500 + (currentElo - 1500) * 0.5`
- 赛季奖励：

| 段位 | 奖励 |
|------|------|
| 黄金+ | 皮肤"金甲虫" |
| 钻石+ | 称号"永恒" |
| 大师 | 光效"烈焰" |
| 王者 | 皮肤"王冠" |

---

## 8. 游戏记录系统

### 8.1 单局记录 (`GameRecord`)

| 字段 | 类型 | 说明 |
|------|------|------|
| recordId | QUuid | 唯一ID |
| timestamp | QDateTime | 游戏时间 |
| duration | int | 游戏时长(秒) |
| finalMass | float | 最终质量 |
| maxMass | float | 最大质量 |
| killCount | int | 击杀数 |
| deathCause | enum | 死因(被吃/掉线/主动退出) |
| killedBy | QString | 被谁击杀 |
| victimNames | QStringList | 击杀了谁 |
| foodEaten | int | 吃食物数 |
| splitCount | int | 分裂次数 |
| ejectCount | int | 吐球次数 |
| eloChange | int | ELO变化 |
| rankInMatch | int | 本局排名 |
| totalPlayers | int | 本局总人数 |
| mode | enum | 游戏模式(单人/联机) |
| seasonId | QString | 所属赛季 |
| massTimeline | QVector<QPair<float,float>> | 质量时间线(时间,质量) |
| killTimeline | QVector<KillEvent> | 击杀时间线 |

### 8.2 玩家累计统计 (`PlayerStats`)

| 字段 | 类型 | 说明 |
|------|------|------|
| totalGames | int | 总局数 |
| totalKills | int | 总击杀 |
| totalDeaths | int | 总死亡 |
| kdaRatio | float | KDA |
| winRate | float | 胜率(排名前3算胜) |
| avgMaxMass | float | 平均最大质量 |
| bestMass | float | 历史最高质量 |
| bestRank | int | 历史最高排名 |
| longestSurvival | int | 最长存活时间(秒) |
| totalFoodEaten | int | 总吃食物数 |
| totalPlayTime | int | 总游戏时间(秒) |
| currentStreak | int | 当前连胜/连败 |
| bestStreak | int | 最长连胜 |
| recentTrend | enum | 近期趋势(上升/平稳/下降) |

### 8.3 SQLite表结构

```sql
CREATE TABLE player_profile (
    player_id   TEXT PRIMARY KEY,
    name        TEXT NOT NULL,
    elo         INTEGER DEFAULT 1000,
    rank_tier   TEXT DEFAULT 'Bronze_III',
    season_id   TEXT,
    created_at  DATETIME,
    updated_at  DATETIME
);

CREATE TABLE game_record (
    record_id     TEXT PRIMARY KEY,
    player_id     TEXT,
    timestamp     DATETIME,
    duration      INTEGER,
    final_mass    REAL,
    max_mass      REAL,
    kill_count    INTEGER,
    death_cause   TEXT,
    killed_by     TEXT,
    food_eaten    INTEGER,
    split_count   INTEGER,
    eject_count   INTEGER,
    elo_change    INTEGER,
    rank_in_match INTEGER,
    total_players INTEGER,
    mode          TEXT,
    season_id     TEXT,
    FOREIGN KEY(player_id) REFERENCES player_profile(player_id)
);

CREATE TABLE kill_detail (
    id            INTEGER PRIMARY KEY AUTOINCREMENT,
    record_id     TEXT,
    victim_name   TEXT,
    kill_time     REAL,
    victim_mass   REAL,
    is_split_kill INTEGER DEFAULT 0,
    FOREIGN KEY(record_id) REFERENCES game_record(record_id)
);

CREATE TABLE mass_timeline (
    id          INTEGER PRIMARY KEY AUTOINCREMENT,
    record_id   TEXT,
    time_sec    REAL,
    mass        REAL,
    FOREIGN KEY(record_id) REFERENCES game_record(record_id)
);

CREATE TABLE season_record (
    season_id     TEXT PRIMARY KEY,
    player_id     TEXT,
    start_date    DATETIME,
    end_date      DATETIME,
    peak_elo      INTEGER,
    peak_tier     TEXT,
    games_played  INTEGER,
    final_elo     INTEGER
);

CREATE TABLE unlock (
    id          INTEGER PRIMARY KEY AUTOINCREMENT,
    player_id   TEXT,
    item_type   TEXT,
    item_id     TEXT,
    unlocked_at DATETIME,
    source      TEXT
);

CREATE TABLE player_stats (
    player_id         TEXT PRIMARY KEY,
    total_games       INTEGER DEFAULT 0,
    total_kills       INTEGER DEFAULT 0,
    total_deaths      INTEGER DEFAULT 0,
    best_mass         REAL DEFAULT 0,
    best_rank         INTEGER DEFAULT 999,
    longest_survival  INTEGER DEFAULT 0,
    total_food_eaten  INTEGER DEFAULT 0,
    total_play_time   INTEGER DEFAULT 0,
    best_streak       INTEGER DEFAULT 0,
    current_streak    INTEGER DEFAULT 0,
    FOREIGN KEY(player_id) REFERENCES player_profile(player_id)
);
```

---

## 9. 成就系统

### 9.1 成就定义

| ID | 名称 | 条件 | 奖励类型 | 奖励 |
|----|------|------|---------|------|
| first_game | 初出茅庐 | 完成第一局 | 称号 | "萌新" |
| glutton | 大胃王 | 单局吃100食物 | 皮肤 | "贪吃" |
| slaughter | 屠杀者 | 单局10杀 | 称号 | "杀手" |
| behemoth | 巨无霸 | 质量达到1000 | 皮肤 | "巨人" |
| immortal | 不死之身 | 存活10分钟 | 光效 | "金身" |
| rank_up | 段位晋升 | 首次达到黄金 | 皮肤 | "金甲虫" |
| veteran | 百战老兵 | 游玩100局 | 称号 | "老兵" |
| streak_king | 连胜之王 | 5连胜 | 描边特效 | "烈焰边" |
| split_master | 分裂大师 | 分裂击杀累计50次 | 称号 | "影分身" |
| virus_hunter | 病毒猎人 | 用病毒击杀累计20次 | 皮肤 | "毒刺" |
| top1 | 冠军之路 | 排名第一累计10次 | 称号 | "王者" |
| elo_2000 | 两千分 | ELO达到2000 | 皮肤 | "钻光" |
| mass_2000 | 超级巨无霸 | 质量达到2000 | 光效 | "星云" |
| no_death | 完美一局 | 零死亡且排名第一 | 称号 | "不败" |
| marathon | 马拉松 | 累计游戏10小时 | 称号 | "马拉松" |

### 9.2 成就存储格式 (`definitions.json`)

```json
[
  {
    "id": "first_game",
    "name": "初出茅庐",
    "description": "完成你的第一局游戏",
    "condition": { "type": "total_games", "value": 1 },
    "reward": { "type": "title", "id": "newbie" },
    "icon": "star",
    "hidden": false
  },
  {
    "id": "glutton",
    "name": "大胃王",
    "description": "单局吃掉100个食物",
    "condition": { "type": "food_eaten_single", "value": 100 },
    "reward": { "type": "skin", "id": "glutton" },
    "icon": "food",
    "hidden": false
  }
]
```

---

## 10. 输入系统

### 10.1 动作枚举

```cpp
enum class Action {
    MoveUp,
    MoveDown,
    MoveLeft,
    MoveRight,
    Split,
    Eject,
    ZoomIn,
    ZoomOut,
    Pause,
    ToggleMinimap,
    Chat,
    QuickChat1,
    QuickChat2,
    QuickChat3,
    ToggleHUD,
    Screenshot,
    ActionCount
};
```

### 10.2 键位绑定数据结构

```cpp
struct KeyBinding {
    Action action;
    QKeySequence primary;       // 主键位
    QKeySequence secondary;     // 副键位(可选)
    bool modifiable;            // 是否允许用户修改
    QString displayName;        // 显示名
    QString category;           // 分类
};
```

### 10.3 预设方案

| 预设 | 分裂 | 吐球 | 移动方式 | 说明 |
|------|------|------|---------|------|
| 经典 | Space | E | 鼠标跟随+WASD辅助 | 默认方案 |
| 纯鼠标 | Space | RMB | 鼠标跟随 | 右键吐球，单手可玩 |
| 键盘优先 | Space | E | WASD主移动 | 鼠标仅定向 |
| 左手 | Space | Q | 鼠标跟随+WASD辅助 | 吐球改Q，适合左撇子 |

### 10.4 移动模式

```cpp
enum class MoveMode {
    FollowCursor,    // 鼠标跟随方向(默认)，WASD辅助
    WASDPrimary,     // WASD主移动，鼠标仅指定分裂/吐球方向
    Hybrid           // 两者同时生效，向量叠加
};
```

### 10.5 默认键位绑定

| 动作 | 主键位 | 副键位 | 分类 |
|------|--------|--------|------|
| 向上移动 | W | Up | 移动 |
| 向下移动 | S | Down | 移动 |
| 向左移动 | A | Left | 移动 |
| 向右移动 | D | Right | 移动 |
| 分裂 | Space | - | 操作 |
| 吐球 | E | - | 操作 |
| 放大视野 | ScrollUp | = | 视角 |
| 缩小视野 | ScrollDown | - | 视角 |
| 暂停 | Esc | - | 界面(固定) |
| 小地图 | Tab | M | 界面 |
| 聊天 | Enter | T | 界面 |
| 隐藏HUD | H | - | 界面 |
| 截图 | F12 | - | 界面 |

### 10.6 配置存储格式 (`keybindings.json`)

```json
{
  "preset": "classic",
  "bindings": {
    "MoveUp":       { "primary": "W",          "secondary": "Up" },
    "MoveDown":     { "primary": "S",          "secondary": "Down" },
    "MoveLeft":     { "primary": "A",          "secondary": "Left" },
    "MoveRight":    { "primary": "D",          "secondary": "Right" },
    "Split":        { "primary": "Space",      "secondary": "" },
    "Eject":        { "primary": "E",          "secondary": "" },
    "ZoomIn":       { "primary": "ScrollUp",   "secondary": "=" },
    "ZoomOut":      { "primary": "ScrollDown", "secondary": "-" },
    "Pause":        { "primary": "Esc",        "secondary": "" },
    "ToggleMinimap":{ "primary": "Tab",        "secondary": "M" },
    "Chat":         { "primary": "Enter",      "secondary": "T" },
    "ToggleHUD":    { "primary": "H",          "secondary": "" },
    "Screenshot":   { "primary": "F12",        "secondary": "" }
  },
  "mouse": {
    "moveMode": "followCursor",
    "sensitivity": 1.0,
    "deadZoneFactor": 0.5,
    "invertY": false
  }
}
```

### 10.7 输入上下文 (`InputContext`)

| 上下文 | 生效范围 | 说明 |
|--------|---------|------|
| Menu | 主菜单/设置等 | 仅快捷键生效，WASD不触发移动 |
| Game | 游戏进行中 | 全部绑定生效 |
| Chat | 聊天输入框聚焦时 | 字母键用于文本输入，仅Esc/Enter等系统键生效 |

### 10.8 事件处理流程

```
QKeyEvent / QMouseEvent
        |
        v
InputManager::handleEvent()
        |
        +-- 判断当前 InputContext
        |
        +-- 查询 KeyBindingConfig::actionForKey(key)
        |     返回: Action
        |
        +-- 发射信号 actionTriggered(Action, pressed/released)
        |
        +-- GameEngine 接收 -> 执行对应逻辑
```

---

## 11. 网络模块

### 11.1 架构

- 服务器权威：所有游戏逻辑在服务器计算
- 客户端预测：本地预测移动，服务器校正
- WebSocket通信：JSON文本消息(开发期) / 二进制(优化期)

### 11.2 协议消息 (`Protocol`)

```json
// 客户端 -> 服务器
{
  "type": "input",
  "direction": { "x": 0.5, "y": 0.3 },
  "split": false,
  "eject": false,
  "seq": 12345
}

// 服务器 -> 客户端 (状态同步)
{
  "type": "state",
  "tick": 9876,
  "players": [
    { "id": "p1", "cells": [{"x":100,"y":200,"mass":50}], "name": "Alice" }
  ],
  "food": [ {"x":300,"y":400,"color":"#ff0000"} ],
  "seq": 12345
}

// 服务器 -> 客户端 (增量更新)
{
  "type": "delta",
  "tick": 9877,
  "changes": [
    { "entity": "player", "id": "p1", "field": "cells", "value": [...] }
  ]
}

// 聊天
{ "type": "chat", "from": "Alice", "message": "你好" }

// 加入/离开
{ "type": "join", "name": "Bob", "elo": 1200 }
{ "type": "leave", "id": "p1" }
```

### 11.3 状态同步策略

- 全量快照：每5秒发送一次完整世界状态
- 增量更新：每秒20次，仅发送变化数据
- 客户端插值：接收两个状态之间平滑过渡
- 延迟补偿：服务器记录历史状态，回滚验证玩家输入
- 断线重连：UUID标识，30秒内可重连恢复

---

## 12. 界面设计

### 12.1 整体布局架构

```
+-----------------------------------------------------------------+
|  MainWindow (QMainWindow)                                        |
|  +-------------------------------------------------------------+ |
|  |  QOpenGLWidget (游戏世界 - 全屏渲染)                           | |
|  |  +----------+              +------------------+               | |
|  |  | 左上HUD  |              |   右上排行榜     |               | |
|  |  | 分数/质量|              |   Top 10 列表    |               | |
|  |  +----------+              +------------------+               | |
|  |                    （玩家球体区域）                              | |
|  |  +----------+              +------------------+               | |
|  |  | 左下聊天  |              |  右下小地图      |               | |
|  |  | (联机时)  |              |  150x150px       |               | |
|  |  +----------+              +------------------+               | |
|  +-------------------------------------------------------------+ |
+-----------------------------------------------------------------+
```

### 12.2 主菜单界面

```
+-----------------------------------------------------------------+
|                                                                   |
|             球 球 大 作 战                                         |
|             AGAR.IO CLONE                                         |
|                                                                   |
|         +------------------------------------------+              |
|         | 输入昵称: [____________________]         |              |
|         +------------------------------------------+              |
|                                                                   |
|    +----------+  +----------+  +----------+                       |
|    | 单人开始  |  | 联机对战  |  | 段位赛季  |                    |
|    +----------+  +----------+  +----------+                       |
|    +----------+  +----------+  +----------+                       |
|    | 皮肤商店  |  | 设置     |  | 记录统计  |                    |
|    +----------+  +----------+  +----------+                       |
|                                                                   |
|  +------------------+        +-------------------+                |
|  | 青铜 III         |        | S1 赛季           |                |
|  | ELO: 850         |        | 剩余28天          |                |
|  +------------------+        +-------------------+                |
+-----------------------------------------------------------------+
```

背景: OpenGL渲染的动态球体粒子飘浮动画
按钮: 圆角矩形，hover时脉冲缩放效果
段位徽章: 左下角常驻，带段位专属颜色光晕

### 12.3 游戏内HUD

```
+------------------------------------------------------------------+
| +-----------------+                    +-------------------+       |
| | 质量: 256       |                    | 排行榜            |       |
| | 击杀: 5         |                    | 1. AAAA 1200     |       |
| | #3 / 30人       |                    | 2. BBBB 980      |       |
| | ELO: 1250      |                    | 3. >>你<< 856    |       |
| | +------------+ |                    | 4. CCCC 720      |       |
| | | 钻石 II    | | <- 段位小徽章       | 5. DDDD 650      |       |
| | +------------+ |                    +-------------------+       |
| +-----------------+                                               |
|                                                                   |
|             (方向指示箭头 -> 玩家球体)                              |
|                                                                   |
| +-----------------+    存活时间 05:23    +-------------------+    |
| | 聊天           |                       | 小地图            |    |
| | [消息区域]     |                       | +--------------+ |    |
| | [输入框___]    |                       | |  .  .🟢.  . | |    |
| +-----------------+                       | +--------------+ |    |
|                                           | 食物:1860    |    |
|                                           +-------------------+   |
|  +-----------------------------------------------------------+   |
|  | Space分裂 | E吐球 | WASD移动 | Tab地图 | Esc暂停           |   |
|  |            <- 底部操作提示条(可隐藏)                        |   |
|  +-----------------------------------------------------------+   |
+------------------------------------------------------------------+
```

### 12.4 段位界面

```
+------------------------------------------------------------------+
|  <- 返回                    段位赛季                               |
|                                                                   |
|  +----------------------------------------------------------+    |
|  |                    S1 赛季                                 |    |
|  |              剩余 28天 14小时 30分                         |    |
|  +----------------------------------------------------------+    |
|                                                                   |
|  +-------------------+  +-----------------------------------+     |
|  |   钻石 III        |  |  ELO 进度条                       |     |
|  |                   |  |  2200 --------*-------- 2500      |     |
|  |  ELO: 2340       |  |             2340                   |     |
|  |  最高: 2410      |  |  距钻石II还需160                  |     |
|  |  赛季: 45局      |  |                                    |     |
|  |  胜率: 62%       |  |  段位阶梯图:                       |     |
|  +-------------------+  |  王者  3000+                      |     |
|                         |  大师  2500-2999                  |     |
|  +-------------------+  |  钻石  2000-2499                  |     |
|  |  近20局ELO趋势    |  |  铂金  1600-1999                  |     |
|  |     /--\   /      |  |  黄金  1200-1599                  |     |
|  |  --/----\--/--    |  |  白银  800-1199                   |     |
|  |     \             |  |  青铜  0-799                      |     |
|  +-------------------+  +-----------------------------------+     |
|                                                                   |
|  +----------------------------------------------------------+    |
|  赛季奖励: 黄金+ -> 皮肤"金甲虫" / 钻石+ -> 称号"永恒"         |    |
|           大师 -> 光效"烈焰" / 王者 -> 皮肤"王冠"             |    |
|  +----------------------------------------------------------+    |
+------------------------------------------------------------------+
```

### 12.5 游戏结算界面

```
+------------------------------------------------------------------+
|                                                                   |
|                    你被吃掉了！                                     |
|                                                                   |
|    +----------+  +----------+  +----------+                       |
|    | 排名      |  | 最大质量  |  | 存活时间  |                    |
|    | #3/30    |  |   856    |  |  05:23   |                    |
|    +----------+  +----------+  +----------+                       |
|    +----------+  +----------+  +----------+                       |
|    | 击杀      |  | 吃食物    |  | ELO变化   |                    |
|    |    7      |  |   234    |  |  +32 ↑   |                    |
|    +----------+  +----------+  +----------+                       |
|                                                                   |
|    +--------------------------------------------------------+    |
|    |  钻石 III ---------*-------- 钻石 II                    |    |
|    |        2340        +160->    2500                      |    |
|    +--------------------------------------------------------+    |
|                                                                   |
|         [ 再来一局 ]           [ 返回主菜单 ]                      |
+------------------------------------------------------------------+
```

### 12.6 设置界面 - 键位绑定页

```
+------------------------------------------------------------------+
|  <- 返回                    设置 > 键位绑定                       |
|                                                                   |
|  预设方案:  [经典 v]  [恢复默认]  [导入] [导出]                   |
|                                                                   |
|  -- 移动 -----------------------------------------------------    |
|  鼠标移动模式  o跟随光标  oWASD为主  o混合模式                     |
|  鼠标灵敏度    -----*-------- 1.0x                                |
|  死区大小      ----*--------- 0.5                                 |
|                                                                   |
|  操作        主键位        副键位                                  |
|  向上移动    [  W  ]      [  Up  ]                                |
|  向下移动    [  S  ]      [ Down ]                                |
|  向左移动    [  A  ]      [ Left ]                                |
|  向右移动    [  D  ]      [ Right]                                |
|                                                                   |
|  -- 操作 -----------------------------------------------------    |
|  分裂        [Space]      [      ]                                |
|  吐球        [  E  ]      [      ]                                |
|                                                                   |
|  -- 视角 -----------------------------------------------------    |
|  放大视野    [滚轮上]      [  =  ]                                |
|  缩小视野    [滚轮下]      [  -  ]                                |
|                                                                   |
|  -- 界面 -----------------------------------------------------    |
|  暂停        [ Esc ]      [      ]                                |
|  小地图      [ Tab ]      [  M  ]                                |
|  聊天        [Enter]      [  T  ]                                |
|  隐藏HUD     [  H  ]      [      ]                                |
|  截图        [ F12 ]      [      ]                                |
+------------------------------------------------------------------+
```

### 12.7 记录与统计界面

```
+------------------------------------------------------------------+
|  <- 返回                    游戏记录 & 统计                        |
|                                                                   |
|  +------+ +------+ +------+ +------+ +------+                     |
|  |总局数 | |总击杀 | | KDA  | | 胜率  | |最高分|                     |
|  | 142  | | 387  | | 2.73 | | 61%  | | 3240 |                     |
|  +------+ +------+ +------+ +------+ +------+                     |
|                                                                   |
|  +---------------------+  +----------------------------+          |
|  |  ELO变化趋势         |  |  质量分布                 |          |
|  |      /\  /--\       |  |    /\                      |          |
|  | ---/--\/---\-      |  |   /  \     /\              |          |
|  |   /           \     |  |  /    \   /  \             |          |
|  |  /             \    |  | /      \ /    \            |          |
|  | 近20局              |  | 0-100  100-500  500+       |          |
|  +---------------------+  +----------------------------+          |
|                                                                   |
|  +----------------------------------------------------------+    |
|  | 历史记录                           [筛选] [排序] [搜索]  |    |
|  | #1  06-08 14:23  质量856  击杀7  #2/30  ELO+32          |    |
|  | #2  06-08 12:10  质量420  击杀3  #5/30  ELO-12          |    |
|  | #3  06-07 22:45  质量1200 击杀12 #1/25  ELO+48          |    |
|  | #4  06-07 20:30  质量180  击杀1  #15/30 ELO-24          |    |
|  +----------------------------------------------------------+    |
|                                                                   |
|  +----------------------------------------------------------+    |
|  成就进度    12/30 已解锁                                         |    |
|  [初出茅庐] [大胃王] [百战老兵] [屠杀者] [不死之身]            |    |
|  +----------------------------------------------------------+    |
+------------------------------------------------------------------+
```

### 12.8 单局详情弹窗

```
+------------------------------------------------------------+
|  对局详情                                              X 关闭 |
|  日期: 2026-06-08 14:23                                     |
|  模式: 单人  |  时长: 05:23  |  排名: #2/30               |
|  ELO变化: +32 (1280 -> 1312)                                |
| ------------------------------------------------------------ |
|  质量变化时间线:                                              |
|  /-----\         /\                                           |
|  /      \---\---/  \---\                                     |
|  0:00     2:00    4:00   5:23                               |
|  峰值: 856 @ 4:12                                           |
| ------------------------------------------------------------ |
|  击杀时间线:                                                  |
|  0:45 吃掉 Player_A (质量120)                                |
|  1:32 吃掉 Bot_17  (质量85)                                  |
|  3:15 吃掉 Player_X (质量340)                                |
|  5:23 被 Player_Y 吃掉 (质量920)                             |
| ------------------------------------------------------------ |
|  数据: 击杀7 | 食物234 | 分裂3 | 吐球12                      |
+------------------------------------------------------------+
```

---

## 13. 视觉规范

| 属性 | 值 |
|------|-----|
| 主题 | 深色模式，背景 #1a1a2e |
| 面板背景 | rgba(22, 33, 62, 0.85) + 圆角12px |
| 主强调色 | #0f3460 / #16213e |
| 文字色 | #e0e0e0 主文字，#a0a0a0 次要文字 |
| 段位色系 | 青铜#CD7F32 白银#C0C0C0 黄金#FFD700 铂金#E5E4E2 钻石#B9F2FF 大师#FF4500 王者#FF0000 |
| 成功色 | #4CAF50 绿色 |
| 危险色 | #f44336 红色 |
| 警告色 | #FF9800 橙色 |
| 字体 | "Microsoft YaHei" / "Segoe UI"，数字用等宽 |
| 动画 | QEasingCurve::OutCubic 默认，OutBack 弹出 |
| 动画时长 | 常规200ms，强调400ms |
| 阴影 | 面板 0 4px 20px rgba(0,0,0,0.5) |

---

## 14. 分阶段实施计划

| 阶段 | 内容 | 覆盖模块 | 预估文件数 |
|------|------|---------|-----------|
| P1 基础框架 | CMake+Qt6窗口+GLWidget+游戏循环+网格渲染 | app, engine, renderer | ~12 |
| P2 核心玩法 | 球体移动+食物+吃+相机+碰撞+HUD | entity, physics, renderer, ui | ~14 |
| P3 进阶机制 | 分裂+吐球+合并+病毒+多Cell管理 | entity, physics | ~8 |
| P4 AI | AI控制器+行为状态+Bot生成 | ai | ~4 |
| P5 视觉增强 | 粒子系统+皮肤+SDF文字+小地图+着色器 | particle, skin, renderer | ~10 |
| P6 音效 | AudioManager+音效集成 | audio | ~2 |
| P7 网络 | WebSocket服务器/客户端+协议+同步+聊天 | network | ~6 |
| P8 数据存储 | SQLite+数据库管理+迁移 | storage | ~3 |
| P9 游戏记录 | 单局记录+累计统计+查询 | record | ~6 |
| P10 段位系统 | ELO+段位+赛季+奖励 | ranking | ~4 |
| P11 成就系统 | 成就定义+解锁检测+通知 | achievement | ~2 |
| P12 统计UI | 仪表盘+记录列表+段位徽章+赛季面板 | ui | ~8 |
| P13 打磨 | 平衡性调整+性能优化+集成测试+bug修复 | 全部 | ~4 |

**总计约85个源文件**
