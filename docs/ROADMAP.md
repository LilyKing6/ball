# BallBattle 开发路线图

> 按优先级排序的后续开发任务。最近一次完成：**服务端 AI 填充**。

---

## 当前状态

- ✅ 单机模式完整：自由、极速、团战、大逃杀
- ✅ AI 对手系统
- ✅ M1 网络联机：Go 服务端 + Qt 客户端可连通对战
- ✅ 数据持久化：SQLite + 记录 + 段位 + 成就
- ✅ UI 美化：主菜单、弹窗、HUD 已统一风格
- ✅ 网络模式服务端 AI 填充：默认房间 10 个 AI，行为含漫游/吃食物/追击/逃跑/分裂击杀
- ⚠️ 网络模式仍为最小可用：单房间、无插值、无视野裁剪

---

## 高优先级

### 1. 客户端 Snapshot 插值
**目标**：平滑 30Hz 服务端快照，减少卡顿感。

**涉及文件**：
- `src/renderer/GLWidget.cpp`
- `src/engine/WorldSnapshot.h`

**验收标准**：
- 保留最近 2 帧快照及接收时间
- 渲染时按当前时间在两帧快照间线性插值玩家/实体位置
- 本地玩家输入响应仍保持即时

---

### 3. 服务端视野裁剪
**目标**：仅向客户端发送其视野范围内的实体，降低带宽。

**涉及文件**：
- `server/internal/world/snapshot.go`
- `server/internal/server/client.go`

**验收标准**：
- 以玩家质心为中心，按视野半径过滤食物/病毒/孢子/其他玩家
- 全量快照频率降低或改为增量
- 联机体验无退化

---

## 中优先级

### 3. 多房间 / 大厅系统
**目标**：支持创建/加入多个房间，明确模式选择。

**涉及文件**：
- `server/internal/server/hub.go`
- `server/internal/server/room.go`
- `src/ui/ModeSelectWindow.cpp`

**验收标准**：
- 客户端可选择房间或创建房间
- 服务端支持多个 Room 实例并行运行
- 房间生命周期管理（空房间自动回收）

---

### 4. 成就计数补齐
**目标**：使“分裂大师”“病毒猎人”等成就能够真正解锁。

**涉及文件**：
- `src/engine/GameEngine.cpp`
- `src/achievement/AchievementManager.cpp`
- `src/record/RecordManager.cpp`

**验收标准**：
- 记录单局分裂击杀次数、病毒击杀次数
- 累计统计写入数据库
- 成就解锁检测正确触发

---

### 5. 输入系统与键位绑定
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

## 低优先级

### 6. 段位“超神"
**目标**：与 Wiki 对齐，增加王者之上的“超神”段位。

**涉及文件**：
- `src/ranking/RankSystem.h/cpp`
- `src/ui/RankWindow.cpp`

---

### 7. 刺球变种
**目标**：添加爆裂刺球、大刺球、毒刺球等。

**涉及文件**：
- `src/entity/Virus.h/cpp`
- `src/engine/World.cpp`
- `src/renderer/VirusRenderer.cpp`

---

### 8. 队友吞噬逻辑
**目标**：团战模式下队友间可合作吞噬（保留最后一个球）。

**涉及文件**：
- `src/physics/PhysicsEngine.cpp`

---

## 建议的下一步

如果继续推进，推荐顺序为：

1. **客户端 Snapshot 插值** → 提升联机流畅度
2. **成就计数补齐** → 完善现有系统闭环
3. **多房间 / 大厅** → 为多人联机扩展做准备
4. **输入系统** → 提升用户体验
5. **服务端视野裁剪** → 优化带宽（可与插值并行）

---

## 参考文档

- 架构设计：[`DESIGN.md`](DESIGN.md)
- 网络迁移：[`M1_NETWORK_MIGRATION.md`](M1_NETWORK_MIGRATION.md)
- Wiki 对照：[`WIKI_RESEARCH.md`](WIKI_RESEARCH.md)
- 视觉增强：[`P5_VISUAL_ENHANCEMENT.md`](P5_VISUAL_ENHANCEMENT.md)
- 历史方案：`archive/`
