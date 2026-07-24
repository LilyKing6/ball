#include "Camera.h"
#include "util/Config.h"
#include <cmath>

Camera::Camera() {
    m_proj.setToIdentity();
    m_view.setToIdentity();
    m_pos = {3000.0f, 3000.0f};
    m_targetPos = {3000.0f, 3000.0f};
    m_smoothing = Config::instance().cameraSmoothing;
}

void Camera::setTarget(const Vec2& pos, float zoom) {
    m_targetPos = pos;
    m_targetZoom = zoom;
}

void Camera::updateProjection(int viewportW, int viewportH) {
    if (viewportW <= 0 || viewportH <= 0) return;

    float aspect = float(viewportW) / float(viewportH);
    // Match QPainter: 1 world unit = 3 pixels at zoom=1.0
    // Projection is zoom-independent; view matrix handles zoom
    float halfH = viewportH / 6.0f;
    float halfW = halfH * aspect;

    m_proj.setToIdentity();
    m_proj.ortho(-halfW, halfW, -halfH, halfH, -1.0f, 1.0f);
}

void Camera::update(float dt) {
    float t = 1.0f - powf(1.0f - m_smoothing, dt * 60.0f);
    m_pos = Vec2::lerp(m_pos, m_targetPos, t);
    m_zoom = lerp(m_zoom, m_targetZoom, t);

    m_view.setToIdentity();
    m_view.scale(m_zoom, m_zoom, 1.0f);
    m_view.translate(-m_pos.x, -m_pos.y, 0.0f);
}

Vec2 Camera::screenToWorld(const QPointF& screenPos, const QSize& viewport) const {
    float sx = 2.0f * screenPos.x() / viewport.width() - 1.0f;
    float sy = 1.0f - 2.0f * screenPos.y() / viewport.height();

    float aspect = float(viewport.width()) / viewport.height();
    float halfH = viewport.height() / 6.0f;
    float halfW = halfH * aspect;

    float wx = m_pos.x + sx * halfW / m_zoom;
    float wy = m_pos.y + sy * halfH / m_zoom;
    return {wx, wy};
}

QPointF Camera::worldToScreen(const Vec2& worldPos, const QSize& viewport) const {
    float aspect = float(viewport.width()) / viewport.height();
    float halfH = viewport.height() / 6.0f;
    float halfW = halfH * aspect;

    float vx = m_zoom * (worldPos.x - m_pos.x);
    float vy = m_zoom * (worldPos.y - m_pos.y);

    float sx = vx / halfW;
    float sy = vy / halfH;

    float px = (sx + 1.0f) * 0.5f * viewport.width();
    float py = (1.0f - sy) * 0.5f * viewport.height();
    return {px, py};
}
