#include "VirtualJoystick.h"
#include <QPainter>
#include <cmath>

VirtualJoystick::VirtualJoystick(QWidget* parent) : QWidget(parent) {
    setAttribute(Qt::WA_TransparentForMouseEvents, true);
    setStyleSheet("background: transparent;");
    m_basePos = {0, 0};
    m_stickPos = {0, 0};
}

void VirtualJoystick::handlePress(const QPointF& screenPos) {
    m_pressStart = screenPos;
    m_activated = false;
    if (m_floating) {
        m_basePos = screenPos;
    }
    m_stickPos = m_basePos;
    m_stickVec = {0, 0};
    m_active = true;
    update();
}

void VirtualJoystick::handleMove(const QPointF& screenPos) {
    if (!m_active) return;

    QPointF delta = screenPos - m_pressStart;
    float deltaLen = std::sqrt(delta.x() * delta.x() + delta.y() * delta.y());

    if (!m_activated) {
        if (deltaLen < m_deadzone) return;  // 防误触
        m_activated = true;
    }

    QPointF vec = screenPos - m_basePos;
    float len = std::sqrt(vec.x() * vec.x() + vec.y() * vec.y());
    if (len > m_radius) {
        vec *= (m_radius / len);
        len = m_radius;
    }

    m_stickVec = vec;
    m_stickPos = m_basePos + vec;
    update();

    // 屏幕 Y 向下正，世界 Y 向上正：转 worldDir 时反 Y
    QPointF worldDir(vec.x() / m_radius, -vec.y() / m_radius);
    emit stickMoved(worldDir);
}

void VirtualJoystick::handleRelease() {
    m_active = false;
    m_activated = false;
    m_stickVec = {0, 0};
    m_stickPos = m_basePos;
    update();
    emit stickReleased();
}

void VirtualJoystick::paintEvent(QPaintEvent*) {
    if (m_floating && !m_active) return;

    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    // 摇杆底盘
    p.setPen(QPen(QColor(255, 255, 255, 80), 2));
    p.setBrush(QColor(0, 0, 0, 80));
    p.drawEllipse(m_basePos, m_radius, m_radius);

    // 中心十字
    p.setPen(QPen(QColor(255, 255, 255, 60), 1));
    p.drawLine(m_basePos.x() - 8, m_basePos.y(), m_basePos.x() + 8, m_basePos.y());
    p.drawLine(m_basePos.x(), m_basePos.y() - 8, m_basePos.x(), m_basePos.y() + 8);

    // 把手
    QPointF handle = m_active ? m_stickPos : m_basePos;
    QColor handleColor = m_active ? QColor(255, 215, 0, 180) : QColor(180, 180, 180, 120);
    p.setBrush(handleColor);
    p.setPen(QPen(handleColor.darker(150), 2));
    p.drawEllipse(handle, m_radius * 0.35f, m_radius * 0.35f);
}
