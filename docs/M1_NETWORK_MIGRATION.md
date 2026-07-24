# M1 网络化迁移开发文档

> 本文档记录 BallBattle 从单机 Qt 游戏向客户端/服务端架构迁移的 M1 阶段工作。
> M1 目标：最小可用网络版（单房间自由模式，移动 + 吃食物 + 玩家间吞噬）。

---

> Verification update (2026-07-24): `go test -race ./... -count=1` passes. The server now spawns 10 AI players by default in the free-mode room. AI behavior includes wander, seek food, chase smaller players, flee bigger players, and split-kill, ported from the Qt `AIController`. The Qt client now keeps the last two snapshots and linearly interpolates player cell positions/masses between them, smoothing the 30Hz server updates. Live verification passed for `/health`, WebSocket `join -> welcome -> snapshot`, a Python smoke test confirming 11 players (1 human + 10 AI) in the snapshot, and a C++ unit test for `WorldSnapshot::lerp`. Remaining limitations are one fixed `default` room, no account system, and full-world snapshots.

## 1. 总览

| 组件 | 技术栈 | 位置 | 状态 |
|------|--------|------|------|
| Go 服务端 | Go 1.26 + gorilla/websocket | `server/` | ✅ 完整实现，编译通过，已验证 |
| Qt 客户端 | Qt6 + QWebSocket | `src/network/` | ✅ M1 核心闭环完成（移动 + 吃食物 + 吞噬 + 双客户端互见 + 断线遮罩） |

### 当前完成度

**Go 服务端**：核心物理 + 协议 + WebSocket 服务全部完成，可独立运行测试。

**Qt 客户端**：M1 核心闭环已通过联调验证：
- ✅ 单客户端连接服务端进入游戏
- ✅ 双客户端互见对方的球、鼠标跟随移动同步（30Hz snapshot）
- ✅ 吃食物后质量变化同步到另一客户端
- ✅ 玩家间吞噬同步
- ✅ 断线重连遮罩 + 3次退避后弹窗回主菜单
- ✅ 单机模式回归验证通过（模式切换残留清理）

---

## 2. 协议规范（JSON over WebSocket）

### 消息外层结构

所有消息使用统一 `Envelope`：
```json
{"type": "<string>", "payload": <object>}
```

### 客户端 → 服务端

**加入房间**：
```json
{"type": "join", "payload": {"name": "Alice", "mode": "free"}}
```

**玩家输入**（30Hz 节流）：
```json
{
  "type": "input",
  "payload": {
    "input": {
      "cursor": {"x": 1500, "y": 1500},
      "wantSplit": false,
      "wantEject": false,
      "mouseWorld": {"x": 1500, "y": 1500}
    }
  }
}
```

**离开房间**：
```json
{"type": "leave"}
```

### 服务端 → 客户端

**欢迎消息**（加入后立即发送，分配 playerId）：
```json
{
  "type": "welcome",
  "payload": {
    "playerId": 1,
    "worldWidth": 3000,
    "worldHeight": 3000,
    "roomName": "default"
  }
}
```

**世界快照**（30Hz 广播）：
```json
{
  "type": "snapshot",
  "payload": {
    "tickId": 123,
    "snapshot": {
      "tickId": 123,
      "gameTime": 45.6,
      "worldWidth": 3000,
      "worldHeight": 3000,
      "gameMode": 0,
      "safeZoneRadius": 0,
      "safeZoneCenterX": 0,
      "safeZoneCenterY": 0,
      "shrinkPhase": 0,
      "timeToNextShrink": -1,
      "players": [
        {
          "id": 1,
          "name": "Alice",
          "team": 0,
          "shield": 0,
          "isLocal": false,
          "isAlive": true,
          "cells": [{"x": 1500, "y": 1500, "mass": 10}]
        }
      ],
      "foods": [{"x": 200, "y": 300}],
      "viruses": []
    }
  }
}
```

**错误消息**：
```json
{"type": "error", "payload": {"message": "invalid json: ..."}}
```

---

## 3. Go 服务端架构

### 目录结构

```
server/
├── go.mod                          # module ballbattle-server
├── go.sum
├── main.go                         # 入口，HTTP 服务监听 :8765
├── internal/
│   ├── proto/
│   │   └── messages.go             # 协议消息结构（Envelope/Join/Input/Welcome/Snapshot/Error）
│   ├── world/
│   │   ├── config.go               # 游戏常量（移植自 Qt Config.h）
│   │   ├── player.go               # Player + Cell + 移动逻辑
│   │   ├── food.go                 # Food + SpawnFoods/RespawnFoods
│   │   ├── world.go                # World 主循环 + 玩家间吞噬
│   │   └── snapshot.go             # BuildSnapshot 序列化
│   └── server/
│       ├── hub.go                  # Hub 路由消息 + 管理房间
│       ├── room.go                 # 房间 tick loop（物理 60Hz + 广播 30Hz）
│       ├── client.go               # WebSocket 客户端会话（read/write pump）
│       └── http.go                 # HTTP→WS 升级处理
└── ballbattle-server.exe           # 构建产物（go build）
```

### 关键参数

| 参数 | 值 | 说明 |
|------|----|----|
| 监听端口 | 8765 | 可通过 `-addr :PORT` 修改 |
| 物理频率 | 60Hz | `time.NewTicker(time.Second / 60)` |
| 广播频率 | 30Hz | `time.NewTicker(time.Second / 30)` |
| 最大消息大小 | 8192 字节 | 服务端客户端单条消息限制（`client.go`） |
| Ping 间隔 | 54s | WebSocket 心跳 |
| Pong 超时 | 60s | 超时断开连接 |

### 启动方式

```bash
cd D:/Projects/ball/server
go build -buildvcs=false -o ballbattle-server.exe .
./ballbattle-server.exe -addr :8765
```

### 健康检查

```bash
curl http://127.0.0.1:8765/health
# 返回：ok
```

### 默认房间配置

M1 阶段只有一个固定房间 `default`：
- 世界大小：3000×3000
- 食物数量：500
- 初始质量：10
- 基础速度：300
- 最大单 cell 质量：40000

### 当前未实现（后续阶段）

- ✅ 服务端自由模式：分裂、吐球、融合、病毒、大豆
- ✅ AI 填充（服务端自由模式已生成 10 个 AI 玩家，带状态机行为）
- ✅ 客户端 Snapshot 插值（双缓冲 + 玩家 cell 位置/质量线性插值）
- ❌ 团战模式
- ❌ 大逃杀缩圈
- ❌ 完整防护盾系统
- ✅ 真人玩家出生无敌倒计时

### 服务端 AI 填充

M1 阶段服务端默认房间会生成 10 个 AI 玩家，保证单人进入网络模式时也不会空荡。

| 文件 | 说明 |
|------|------|
| `server/internal/world/ai.go` | AI 状态机（Wander/SeekFood/ChaseSmaller/FleeBigger/SplitKill） |
| `server/internal/world/ai_test.go` | AI 单元测试 |
| `server/internal/server/room.go` | 房间 AI 生成、更新、数量维持逻辑 |

AI 行为移植自 Qt 客户端的 `AIController`，难度分 Easy/Normal/Hard：
- Easy：视野小、反应慢、不会分裂
- Normal：视野中等、会分裂但概率低
- Hard：视野大、反应快、分裂击杀积极

AI 玩家使用负数 `playerID`，死亡后由 `world.Step` 自动复活，房间只在总数不足时补充。

### 客户端 Snapshot 插值

为平滑 30Hz 服务端快照，`GLWidget` 维护最近两帧 snapshot：

| 文件 | 说明 |
|------|------|
| `src/engine/WorldSnapshot.h` | `WorldSnapshot::lerp(prev, cur, alpha)`：按玩家 ID 匹配，按 cell 索引线性插值位置/质量 |
| `src/renderer/GLWidget.cpp` | 收到 snapshot 时保留 prev/cur；`paintGL` 按时间 alpha 插值后应用 |
| `tests/TestWorldSnapshotLerp.cpp` | C++ 单元测试，验证基础插值、新增 cell、新增玩家 |

插值策略：
- `alpha = (now - curRecvMs) / (curRecvMs - prevRecvMs)`，clamp 到 `[0, 1]`
- 静态实体（食物、病毒、大豆）直接使用最新 snapshot
- 玩家 cell 按索引插值；新增 cell 直接出现在 cur 位置
- 本地玩家输入响应仍保持即时，不受影响

---

## 4. Qt 客户端改造进度

### 已完成

#### 4.1 Config 新增字段（`src/util/Config.h`）

```cpp
// Network
bool networkMode = false;
QString serverHost = "127.0.0.1";
int serverPort = 8765;
QString playerName = "Player";
```

已在 `Config.cpp` 的 `load()` / `save()` 中加入 `CFG_LOAD` / `CFG_SAVE`，持久化到 `config.json`。

#### 4.2 NetworkClient 新文件（`src/network/NetworkClient.h/cpp`）

基于 `QWebSocket` 的客户端封装：

```cpp
class NetworkClient : public QObject {
    Q_OBJECT
public:
    explicit NetworkClient(QObject* parent = nullptr);
    ~NetworkClient() override;

    void connectTo(const QUrl& url, const QString& name, const QString& mode);
    void disconnect();
    void sendInput(const PlayerInput& input);  // 自动节流到 30Hz
    bool isConnected() const;
    quint64 myPlayerId() const { return m_myId; }

signals:
    void connected();
    void welcomeReceived(quint64 playerId, float worldW, float worldH);
    void snapshotReceived(const WorldSnapshot& snap, qint64 recvMs);
    void disconnected(const QString& reason);
    void errorOccurred(const QString& msg);

private:
    QWebSocket m_socket;
    quint64 m_myId = 0;
    qint64 m_lastSentMs = 0;
    int m_reconnectAttempts = 0;
    QUrl m_url;
    QString m_name, m_mode;
    // ... 内部状态
};
```

特性：
- 自动重连 3 次（1s/2s/4s 退避），失败后 emit `disconnected`
- `sendInput` 节流：两次发送间隔不少于 33ms
- 收到 `welcome` 解析 playerId 并 emit `welcomeReceived`
- 收到 `snapshot` 调用 `WorldSnapshot::fromJson` 并 emit `snapshotReceived`

#### 4.3 GameEngine 网络分支（`src/engine/GameEngine.h/cpp`）

新增成员：
```cpp
bool m_networkMode = false;
NetworkClient* m_networkClient = nullptr;
quint64 m_networkMyId = 0;
PlayerInput m_pendingInput;
```

新增方法：
- `void setNetworkMode(bool on)` / `bool networkMode() const`
- `void applySnapshot(const WorldSnapshot& snap, quint64 myId)` — 转发给 `World::applySnapshot`
- `PlayerInput pendingInput() const` / `void clearPendingInputFlags()`

`update(float dt)` 开头加：
```cpp
if (m_networkMode) return;  // 网络模式跳过本地物理
```

`setLocalPlayerCursor` / `splitLocalPlayer` / `ejectFromLocalPlayer` 网络模式下只更新 `m_pendingInput`，不操作本地 World。

#### 4.4 World::applySnapshot 声明（`src/engine/World.h`）

```cpp
void applySnapshot(const WorldSnapshot& snap, quint64 myId);
```

声明和实现均已完成，当前网络快照会由 `World::applySnapshot` 重建本地实体。

### M1 实施记录（代码已落地）

#### Step 1: World::applySnapshot 实现（已完成，`src/engine/World.cpp`）

```cpp
void World::applySnapshot(const WorldSnapshot& snap, quint64 myId) {
    // 1. 同步世界尺寸
    m_width = snap.worldWidth;
    m_height = snap.worldHeight;

    // 2. 清空本地实体
    m_players.clear();
    m_foods.clear();
    m_viruses.clear();
    m_aiControllers.clear();  // 网络模式无 AI

    // 3. 重建玩家
    m_localPlayerIdx = -1;
    for (const auto& po : snap.players) {
        Player p;
        p.id = po.id;
        p.name = po.name;
        p.team = po.team;
        p.shieldCount = po.shieldCount;
        p.isAI = false;
        p.mouseWorldPos = {0, 0};
        p.virtualCursor = {0, 0};

        for (const auto& co : po.cells) {
            Cell c;
            c.pos = {co.x, co.y};
            c.mass = co.mass;
            c.alive = true;
            c.color = QColor::fromHsv((po.id * 47) % 360, 200, 230);
            p.cells.append(c);
        }

        if (po.id == static_cast<int>(myId)) {
            m_localPlayerIdx = m_players.size();
        }
        m_players.append(p);
    }

    // 4. 重建食物
    for (const auto& fo : snap.foods) {
        Food f;
        f.pos = {fo.x, fo.y};
        f.mass = Config::instance().foodMass;
        f.color = QColor::fromHsv((int(fo.x * 7 + fo.y * 13) % 360), 200, 230);
        f.alive = true;
        m_foods.append(f);
    }

    // 5. 重建病毒
    for (const auto& vo : snap.viruses) {
        Virus v({vo.x, vo.y});
        v.mass = Config::instance().virusMass;
        v.alive = true;
        m_viruses.append(v);
    }

    // 6. 同步游戏时间
    m_gameTime = snap.gameTime;

    // 7. 大逃杀同步
    if (snap.safeZoneRadius > 0) {
        m_safeZoneRadius = snap.safeZoneRadius;
        m_safeZoneCenter = {snap.safeZoneCenterX, snap.safeZoneCenterY};
        m_shrinkPhase = snap.shrinkPhase;
    }
}
```

#### Step 2: GLWidget 改造（已完成，`src/renderer/GLWidget.cpp`）

**2.1 paintGL 跳过本地物理**：

```cpp
// 原代码（约 139-142 行）：
auto& cfg = Config::instance();
m_accumulator += rawDt;
int maxSteps = 5;
while (m_accumulator >= cfg.fixedDt && maxSteps-- > 0) {
    m_engine->update(cfg.fixedDt);
    m_accumulator -= cfg.fixedDt;
}

// 改为：
if (!m_engine->networkMode()) {
    auto& cfg = Config::instance();
    m_accumulator += rawDt;
    int maxSteps = 5;
    while (m_accumulator >= cfg.fixedDt && maxSteps-- > 0) {
        m_engine->update(cfg.fixedDt);
        m_accumulator -= cfg.fixedDt;
    }
}
```

**2.2 持有 NetworkClient + 双缓冲 snapshot**：

`GLWidget.h` 新增：
```cpp
#include "network/NetworkClient.h"
#include "engine/WorldSnapshot.h"

// 在 private 区新增：
NetworkClient* m_net = nullptr;
WorldSnapshot m_prevSnap;
WorldSnapshot m_curSnap;
qint64 m_curRecvMs = 0;
QTimer* m_inputTimer = nullptr;

void onSnapshotReceived(const WorldSnapshot& snap, qint64 recvMs);
void sendInputTick();
```

**2.3 onSnapshotReceived 实现**：
```cpp
void GLWidget::onSnapshotReceived(const WorldSnapshot& snap, qint64 recvMs) {
    m_prevSnap = m_curSnap;
    m_curSnap = snap;
    m_curRecvMs = recvMs;
    m_engine->applySnapshot(snap, m_net->myPlayerId());
    update();  // 触发重绘
}
```

**2.4 30Hz 输入定时器**：
```cpp
void GLWidget::sendInputTick() {
    if (!m_net || !m_net->isConnected()) return;
    m_net->sendInput(m_engine->pendingInput());
    m_engine->clearPendingInputFlags();
}
```

启动时机：网络 session 建立后调用 `m_inputTimer->start(33);`。

**2.5 双缓冲插值渲染**（M1 基础版）：

在 `paintGL` 中渲染玩家时：
```cpp
qint64 now = QDateTime::currentMSecsSinceEpoch();
float alpha = (now - m_curRecvMs) / 33.0f;
alpha = qBound(0.0f, alpha, 1.0f);
// 渲染时用 m_curSnap 位置（M1 先不做插值，直接显示最新 snapshot）
// M5 优化时再做 prev→cur 线性插值
```

#### Step 3: MainWindow 整合（已完成，`src/app/MainWindow.cpp`）

**3.1 连接 ModeSelectWindow 信号**：
```cpp
connect(m_modeSelectWindow, &ModeSelectWindow::networkModeSelected,
        this, &MainWindow::startNetworkedGame);
```

**3.2 startNetworkedGame slot**：
```cpp
void MainWindow::startNetworkedGame(const QString& host, quint16 port, const QString& name) {
    auto& cfg = Config::instance();
    cfg.networkMode = true;
    cfg.serverHost = host;
    cfg.serverPort = port;
    cfg.playerName = name;
    cfg.save("config.json");

    m_menu->hide();
    m_hud->showHUD();

    // 创建 NetworkClient
    auto* net = new NetworkClient(this);
    m_engine->setNetworkClient(net);
    m_glWidget->setNetworkClient(net);

    QUrl url(QString("ws://%1:%2/ws").arg(host).arg(port));
    connect(net, &NetworkClient::welcomeReceived, this, [this](quint64 pid, float w, float h) {
        m_engine->setNetworkMode(true);
        m_glWidget->startNetworkSession();
        qDebug() << "Joined as player" << pid << "world" << w << "x" << h;
    });
    connect(net, &NetworkClient::disconnected, this, [this](const QString& reason) {
        QMessageBox::warning(this, "断线", reason);
        showMainMenu();
    });

    net->connectTo(url, name, "free");
}
```

#### Step 4: ModeSelectWindow UI（已完成，`src/ui/ModeSelectWindow.cpp`）

现有 4 个模式卡片后追加"联网对战"卡片：
```cpp
// 在卡片网格末尾追加
auto* netBtn = new QPushButton("🌐 联网对战", this);
netBtn->setStyleSheet(/* 与其他卡片一致 */);
grid->addWidget(netBtn, row, col);

connect(netBtn, &QPushButton::clicked, this, [this]() {
    bool ok = false;
    QString host = QInputDialog::getText(this, "服务器地址", "Host:", QLineEdit::Normal, "127.0.0.1", &ok);
    if (!ok || host.isEmpty()) return;
    QString portStr = QInputDialog::getText(this, "端口", "Port:", QLineEdit::Normal, "8765", &ok);
    if (!ok || portStr.isEmpty()) return;
    QString name = QInputDialog::getText(this, "玩家名", "Name:", QLineEdit::Normal, "Player", &ok);
    if (!ok || name.isEmpty()) return;

    emit networkModeSelected(host, portStr.toUShort(), name);
});
```

`ModeSelectWindow.h` 新增信号：
```cpp
signals:
    void networkModeSelected(const QString& host, quint16 port, const QString& name);
```

#### Step 5: CMakeLists.txt 更新（已完成）

```cmake
# SOURCES 追加：
src/network/NetworkClient.cpp

# HEADERS 追加：
src/network/NetworkClient.h
```

`Qt6::WebSockets` 已链接，无需修改 `find_package` / `target_link_libraries`。

#### Step 6: 构建验证（已完成）

```bash
# 终端 A：启动 Go 服务端
cd D:/Projects/ball/server
./ballbattle-server.exe -addr :8765

# 终端 B：构建 Qt 客户端
cd D:/Projects/ball
cmake --build build

# 终端 C：启动 Qt 客户端
./build/BallBattle.exe
# 主菜单 → 模式选择 → 联网对战 → 127.0.0.1:8765 + Alice

# 终端 D：第二个 Qt 客户端
./build/BallBattle.exe
# 联网对战 → 127.0.0.1:8765 + Bob
```

预期结果：
- 两个客户端互相看到对方的球
- 鼠标移动球跟随（30Hz snapshot + 无插值，可能有轻微卡顿）
- 吃食物后质量变化同步
- 服务端日志显示 join/input 处理

#### Step 7: 断线处理（已完成，`GLWidget.cpp`）

```cpp
void GLWidget::onDisconnected(const QString& reason) {
    // 显示"重连中..."遮罩
    m_reconnectOverlay = true;
    update();

    // NetworkClient 内部已自动重连 3 次
    // 彻底失败后 emit disconnected(reason) 由 MainWindow 处理回主菜单
}
```

在 `paintGL` 末尾绘制重连遮罩：
```cpp
if (m_reconnectOverlay) {
    QPainter p(this);
    p.fillRect(rect(), QColor(0, 0, 0, 150));
    p.setPen(Qt::white);
    p.setFont(QFont("Microsoft YaHei", 18, QFont::Bold));
    p.drawText(rect(), Qt::AlignCenter, "重连中...");
}
```

---

## 5. 文件清单总览

### 新增文件

| 路径 | 角色 |
|------|------|
| `server/**` | Go 服务端全部代码 |
| `server/go.mod` / `go.sum` | Go 模块定义 |
| `server/main.go` | 服务端入口 |
| `server/internal/proto/messages.go` | 协议消息结构 |
| `server/internal/world/*.go` | 物理世界（玩家/食物/病毒/snapshot） |
| `server/internal/server/*.go` | 网络层（Hub/Room/Client/HTTP） |
| `src/network/NetworkClient.h/cpp` | Qt WebSocket 客户端 |
| `src/engine/IController.h/cpp` | 控制器抽象接口（为 RL 接入预留） |
| `src/engine/HumanController.h` | 人类控制器（未实际使用，保留） |
| `src/engine/WorldSnapshot.h/cpp` | 快照序列化 |
| `src/network/ExternalAgentServer.h/cpp` | 旧版内嵌服务端（RL agent 用，将被 Go 服务端替代） |
| `tools/agent_test_client.py` | Python 测试客户端（旧协议） |
| `tools/m1_test_client.py` | Python 测试客户端（M1 新协议） |

### 修改文件

| 路径 | 改动内容 |
|------|----------|
| `src/util/Config.h/cpp` | 新增 networkMode/serverHost/serverPort/playerName + agentServer 字段 |
| `src/engine/GameEngine.h/cpp` | 新增网络分支、applySnapshot、pendingInput、agentServer 集成 |
| `src/engine/World.h/cpp` | 声明并实现 `applySnapshot(snap, myId)` |
| `CMakeLists.txt` | 新增 IController/WorldSnapshot/ExternalAgentServer/NetworkClient 源文件 |
| `src/entity/Player.h` | 新增 virtualCursor、shieldCount、invincibleTimer、模式倍率字段 |
| `src/engine/GameMode.h` | 新增极速模式倍率、大逃杀缩圈参数 |

### 已修改文件（Step 1-7）

| 路径 | 已完成内容 |
|------|------|
| `src/engine/World.cpp` | 实现 `applySnapshot` |
| `src/renderer/GLWidget.h/cpp` | 网络分支、双缓冲、输入定时器、重连遮罩 |
| `src/app/MainWindow.cpp` | `startNetworkedGame` slot |
| `src/ui/ModeSelectWindow.h/cpp` | 联网对战卡片 + `networkModeSelected` 信号 |
| `CMakeLists.txt` | 加入 `NetworkClient.cpp/h` |

---

## 6. 端到端验证流程

### 单机回归（确保不破坏现有功能）

1. **不启动 Go 服务端**
2. 启动 Qt 客户端
3. 选择任一单机模式（自由/极速/团战/大逃杀）
4. 预期：游戏正常运行，无网络相关异常

### 网络模式连通性

1. 启动 Go 服务端：`cd server && ./ballbattle-server.exe -addr :8765`
2. 启动 Qt 客户端，主菜单 → 模式选择 → "联网对战"
3. 输入 `127.0.0.1:8765` + 玩家名 `Alice`
4. 预期：
   - 客户端进入游戏视图，看到服务端世界的食物和自己的球
   - 鼠标移动 → 球追随（30Hz snapshot，可能有轻微卡顿，M5 加插值）
   - 服务端日志显示 `client X joined room default as player Y`
5. 启动第二个 Qt 客户端 `Bob`：
   - 两个客户端互相看到对方的球
   - 一方吃掉另一方时，两端同步更新

### 断线处理

1. 启动服务端 + 客户端，正常进入游戏
2. 关闭 Go 服务端
3. 预期：
   - Qt 客户端显示"重连中..."灰度遮罩
   - 3 次重连失败后弹窗 → 返回主菜单

---

## 7. 已知限制（M1 范围内）

- 服务端自由模式已支持分裂/吐球/融合/病毒/大豆，并默认填充 10 个 AI 玩家
- 客户端已加入 Snapshot 插值，平滑 30Hz 更新
- 只有一个固定房间 `default`（M3 多房间支持）
- 无账号系统，playerId 由服务端按连接顺序分配（M4 账户系统）
- 无视野裁剪，服务端广播全量 snapshot（M5 性能优化）

---

## 8. 下次会话实施顺序

以下只列当前仍未完成的后续工作：

1. ~~服务端自由模式加入 AI 填充玩家，并为 AI 行为添加规则测试。~~ ✅ 已完成
2. ~~为客户端加入 snapshot 插值。~~ ✅ 已完成
3. 增加房间生命周期、大厅/多房间和明确的模式选择。
4. 为服务端加入视野裁剪。
5. 补齐分裂击杀、病毒击杀等累计统计，使对应成就可解锁。

---

## 9. 关键设计决策记录

| 决策点 | 选择 | 理由 |
|--------|------|------|
| 服务端语言 | Go | 并发简单、生态好、开发效率高，性能足够 |
| 迁移范围 | C（完整业务逻辑） | 一次性彻底迁移，避免 Qt/Go 双份逻辑 |
| 实施节奏 | M1-M5 分阶段 | M1 跑通最小闭环，逐步扩展 |
| 传输协议 | JSON over WebSocket | 调试方便，RL agent 直接可读，M5 再切 MessagePack |
| 玩家 ID 分配 | 服务端自动递增 | 服务端权威，避免冲突 |
| snapshot 广播频率 | 30Hz | 带宽与流畅度折中，M5 用插值补平滑 |
| 客户端入口 | ModeSelectWindow 卡片 | 复用现有 UI 架构 |
| 断线重连 | 3 次退避（1s/2s/4s） | 平衡体验与安全 |
| 插值 | M1 不做，M5 做 | 先跑通闭环，M5 性能优化时再加 |

---

## 10. 相关链接

- 计划文件：`C:\Users\Lily\.claude\plans\ethereal-snuggling-bunny.md`
- Go WebSocket 库：https://github.com/gorilla/websocket
- Qt WebSocket 文档：https://doc.qt.io/qt-6/qwebsocket.html
- 项目根：`D:/Projects/ball/`
- 服务端目录：`D:/Projects/ball/server/`
- Qt 客户端目录：`D:/Projects/ball/src/`
