# P5 视觉增强 — 实施文档

> 基于 QPainter 渲染架构，增强游戏视觉效果

---

## 1. 粒子系统 (`src/particle/`)

### 1.1 Particle.h — 粒子数据结构

```cpp
struct Particle {
    QPointF pos;
    QPointF vel;
    QColor color;
    float life;       // 剩余生命 (0~1)
    float maxLife;    // 总生命
    float size;       // 像素大小
};
```

### 1.2 ParticleSystem — 粒子池管理

| 方法 | 说明 |
|------|------|
| `emitBurst(pos, color, count)` | 圆形爆裂粒子 |
| `emitRing(pos, color, radius, count)` | 环形扩散波 |
| `emitTrail(pos, color)` | 尾迹粒子 |
| `update(dt)` | 更新所有粒子（位置/生命/淡出） |
| `render(painter)` | 用 QPainter 绘制 |

### 1.3 触发场景

| 事件 | 效果 | 粒子数 | 颜色 |
|------|------|--------|------|
| 吃食物 | 小爆裂 | 5~8 | 食物颜色 |
| 分裂 | 环形扩散波 | 12~16 | 球体颜色 |
| 合并 | 吸入汇聚 | 8~10 | 球体颜色 |
| 死亡 | 大爆裂 | 20~30 | 球体颜色 |
| 吐球 | 尾迹 | 3~5 | 橙色 |

---

## 2. 小地图 (`src/renderer/`)

### 2.1 MinimapRenderer

- 右下角 150×150 半透明矩形
- 背景 `rgba(0,0,0,0.5)` + 圆角
- 世界边界：白色细线矩形
- 玩家位置：绿色圆点 + 方向指示
- 其他玩家：红色小点
- 病毒：紫色小三角
- 食物不显示（太多会卡）

### 2.2 渲染方式

```cpp
void render(QPainter& p, const QRect& widgetRect, const World& world);
```

独立于主相机，使用固定缩放比例 `150/6000 = 0.025`。

---

## 3. 球体名字标签

### 3.1 渲染规则

- 球体上方居中绘制名字
- 字体大小 = `max(10, radius * 0.35)`
- 白色文字 + 1px 黑色描边（QPainterPath::addText + stroke）
- 仅当球半径 > 20 时显示

### 3.2 描边实现

```cpp
QPainterPath textPath;
textPath.addText(pos, font, name);
p.setPen(QPen(Qt::black, 2));
p.drawPath(textPath);
p.setPen(Qt::white);
p.fillPath(textPath, Qt::white);
```

---

## 4. 皮肤系统 (`src/skin/`)

### 4.1 SkinDef — 皮肤定义

```cpp
enum SkinType { Solid, Gradient, Striped, Dotted };

struct SkinDef {
    QString id;
    QString name;
    SkinType type;
    QColor primaryColor;
    QColor secondaryColor;
    float patternScale;  // 条纹/波点密度
};
```

### 4.2 SkinManager

| 方法 | 说明 |
|------|------|
| `loadDefaults()` | 加载预设皮肤 |
| `getSkin(id)` | 获取皮肤定义 |
| `applyToPainter(painter, skin, pos, radius)` | 应用皮肤到画笔 |

### 4.3 预设皮肤

| ID | 名称 | 类型 | 颜色 |
|----|------|------|------|
| default | 默认 | Solid | 随机HSV |
| fire | 烈焰 | Gradient | 红→橙 |
| ocean | 海洋 | Gradient | 蓝→青 |
| tiger | 虎纹 | Striped | 橙+黑条纹 |
| dots | 波点 | Dotted | 紫+白波点 |

---

## 5. 视觉润色

### 5.1 球体光晕

- 球体外层绘制半透明大圆
- 半径 = 球半径 × 1.3
- 颜色 = 球颜色 + alpha 0.15

### 5.2 移动拖尾

- 记录最近 5 帧位置
- 绘制渐隐残影（alpha 递减）
- 仅当速度 > 阈值时显示

### 5.3 缩放动画

- 吃食物时球体脉冲缩放
- `displayRadius = radius * (1 + pulseAmount)`
- pulseAmount 从 0.15 衰减到 0

---

## 6. 实施步骤

| 步骤 | 内容 | 文件 |
|------|------|------|
| 5.1 | 粒子系统基础 | `src/particle/Particle.h`, `ParticleSystem.h/cpp` |
| 5.2 | 集成粒子到渲染循环 | 修改 `QPainterGLWidget.cpp` |
| 5.3 | 小地图渲染 | `src/renderer/MinimapRenderer.h/cpp` |
| 5.4 | 球体名字标签 + 光晕 | 修改 `QPainterGLWidget.cpp` |
| 5.5 | 皮肤系统 | `src/skin/SkinManager.h/cpp` |
| 5.6 | 移动拖尾 + 动画润色 | 修改 `QPainterGLWidget.cpp` |
| 5.7 | 更新 CMakeLists + 构建验证 | `CMakeLists.txt` |

**新增约 6 个文件，修改约 3 个文件。**
