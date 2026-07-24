#ifndef CAMERA_H
#define CAMERA_H

#include "util/Math.h"
#include <QMatrix4x4>

class Camera {
public:
    Camera();

    void setWorldSize(float w, float h) { m_worldW = w; m_worldH = h; }
    float worldW() const { return m_worldW; }
    float worldH() const { return m_worldH; }

    void setTarget(const Vec2& pos, float zoom);
    void update(float dt);

    Vec2 screenToWorld(const QPointF& screenPos, const QSize& viewport) const;
    QPointF worldToScreen(const Vec2& worldPos, const QSize& viewport) const;

    const QMatrix4x4& projection() const { return m_proj; }
    const QMatrix4x4& view() const { return m_view; }
    float zoom() const { return m_zoom; }

    void updateProjection(int viewportW, int viewportH);

private:
    Vec2 m_pos, m_targetPos;
    float m_zoom = 1.0f, m_targetZoom = 1.0f;
    float m_smoothing = 0.1f;
    float m_worldW = 6000.0f, m_worldH = 6000.0f;
    QMatrix4x4 m_proj, m_view;
};

#endif // CAMERA_H
