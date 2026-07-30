# BallBattle 开发路线图

> 按优先级排序的后续开发任务。最近一次完成：**段位"超神"**。

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
- ✅ 输入系统与键位绑定：可配置分裂/吐球/调试/控制模式按键
- ✅ 服务端视野裁剪：按玩家质心和半径过滤实体，降低带宽
- ✅ 段位"超神"：ELO 3500+ 最高段位纳入检测与 UI
- ⚠️ 网络模式已较完善：剩余为内容增强

---

## 中优先级

### 1. 刺球变种
**目标**：添加爆裂刺球、大刺球、毒刺球等。

**涉及文件**：
- `src/entity/Virus.h/cpp`
- `src/engine/World.cpp`
- `src/renderer/VirusRenderer.cpp`

---

### 3. 队友吞噬逻辑
**目标**：团战模式下队友间可合作吞噬（保留最后一个球）。

**涉及文件**：
- `src/physics/PhysicsEngine.cpp`

---

## 建议的下一步

如果继续推进，推荐顺序为：

1. **刺球变种** → 丰富游戏内容
2. **队友吞噬逻辑** → 团战模式增强

---

## 参考文档

- 架构设计：[`DESIGN.md`](DESIGN.md)
- 网络迁移：[`M1_NETWORK_MIGRATION.md`](M1_NETWORK_MIGRATION.md)
- Wiki 对照：[`WIKI_RESEARCH.md`](WIKI_RESEARCH.md)
- 视觉增强：[`P5_VISUAL_ENHANCEMENT.md`](P5_VISUAL_ENHANCEMENT.md)
- 历史方案：`archive/`
