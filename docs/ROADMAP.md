# BallBattle 开发路线图

> 按优先级排序的后续开发任务。最近一次完成：**多房间 / 大厅系统**。

---

## 当前状态

- ✅ 单机模式完整：自由、极速、团战、大逃杀
- ✅ AI 对手系统
- ✅ M1 网络联机：Go 服务端 + Qt 客户端可连通对战
- ✅ 数据持久化：SQLite + 记录 + 段位 + 成就
- ✅ UI 美化：主菜单、弹窗、HUD 已统一风格
- ✅ 网络模式服务端 AI 填充：默认房间 10 个 AI
- ✅ 客户端 Snapshot 插值：双缓冲 + 玩家 cell 插值
- ✅ 成就计数补齐：分裂击杀 / 病毒击杀累计与解锁
- ✅ 多房间 / 大厅系统：创建、列出、加入房间，空房间自动回收
- ⚠️ 网络模式仍为最小可用：无视野裁剪

---

## 高优先级

### 1. 输入系统与键位绑定
**目标**：实现可配置的键位绑定，替代硬编码按键。

**涉及文件**：
- 新建 `src/input/InputManager.h/cpp`
- 新建 `src/input/KeyBinding.h/cpp`
- `src/ui/SettingsWindow.cpp`
- `src/renderer/GLWidget.cpp`

**验收标准**：
- 支持修改分裂、吐球、移动、HUD 等按键
- 配置持久化到 `config.json`
- 不影响现有默认操作

---

## 中优先级

### 2. 服务端视野裁剪
**目标**：仅向客户端发送其视野范围内的实体，降低带宽。

**涉及文件**：
- `server/internal/world/snapshot.go`
- `server/internal/server/client.go`

**验收标准**：
- 以玩家质心为中心，按视野半径过滤食物/病毒/孢子/其他玩家
- 全量快照频率降低或改为增量
- 联机体验无退化

---

### 3. 段位“超神"
**目标**：与 Wiki 对齐，增加王者之上的“超神”段位。

**涉及文件**：
- `src/ranking/RankSystem.h/cpp`
- `src/ui/RankWindow.cpp`

---

### 4. 刺球变种
**目标**：添加爆裂刺球、大刺球、毒刺球等。

**涉及文件**：
- `src/entity/Virus.h/cpp`
- `src/engine/World.cpp`
- `src/renderer/VirusRenderer.cpp`

---

### 5. 队友吞噬逻辑
**目标**：团战模式下队友间可合作吞噬（保留最后一个球）。

**涉及文件**：
- `src/physics/PhysicsEngine.cpp`

---

## 建议的下一步

如果继续推进，推荐顺序为：

1. **输入系统** → 提升用户体验
2. **服务端视野裁剪** → 优化带宽
3. **段位“超神"** → 完善 Wiki 对齐内容

---

## 参考文档

- 架构设计：[`DESIGN.md`](DESIGN.md)
- 网络迁移：[`M1_NETWORK_MIGRATION.md`](M1_NETWORK_MIGRATION.md)
- Wiki 对照：[`WIKI_RESEARCH.md`](WIKI_RESEARCH.md)
- 视觉增强：[`P5_VISUAL_ENHANCEMENT.md`](P5_VISUAL_ENHANCEMENT.md)
- 历史方案：`archive/`
