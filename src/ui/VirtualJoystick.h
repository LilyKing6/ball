#ifndef VIRTUALJOYSTICK_H
#define VIRTUALJOYSTICK_H

#include <QWidget>
#include <QPointF>
#include <QColor>

// 虚拟摇杆 UI 组件
// 显示在屏幕一角，鼠标拖动控制虚拟游标方向
class VirtualJoystick : public QWidget {
    Q_OBJECT
public:
    explicit VirtualJoystick(QWidget* parent = nullptr);

    void setRadius(float r) { m_radius = r; update(); }
    float radius() const { return m_radius; }

    // 当前摇杆向量（已归一化为半径单位的世界方向）
    QPointF stickVector() const { return m_stickVec; }
    bool isActive() const { return m_active; }

    // 设置位置（屏幕坐标）
    void setBasePosition(const QPointF& pos) { m_basePos = pos; update(); }
    QPointF basePosition() const { return m_basePos; }

    // 设置是否浮动（true = 首次按下位置为摇杆中心）
    void setFloating(bool f) { m_floating = f; }
    bool isFloating() const { return m_floating; }

    // 鼠标事件入口（由 GLWidget 转发）
    void handlePress(const QPointF& screenPos);
    void handleMove(const QPointF& screenPos);
    void handleRelease();

    // 防误触阈值（按下后位移小于此值不算激活）
    void setDeadzone(float d) { m_deadzone = d; }
    float deadzone() const { return m_deadzone; }

signals:
    void stickMoved(QPointF worldDir);  // 摇杆方向变化（worldDir 已乘以半径）
    void stickReleased();

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    QPointF m_basePos;       // 摇杆中心（屏幕坐标）
    QPointF m_stickPos;      // 把手当前位置
    QPointF m_stickVec;      // 相对 base 的偏移（屏幕像素）
    float m_radius = 80.0f;  // 显示半径（屏幕像素）
    float m_deadzone = 10.0f;
    bool m_active = false;
    bool m_floating = false; // 浮动模式：首次按下处为 base
    QPointF m_pressStart;    // 防误触：按下位置
    bool m_activated = false; // 是否已突破 deadzone
};

#endif // VIRTUALJOYSTICK_H
