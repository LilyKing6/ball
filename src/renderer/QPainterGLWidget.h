#ifndef QPAINTERGLWIDGET_H
#define QPAINTERGLWIDGET_H

#include <QWidget>
#include <QPointF>
#include <QVector>
#include "particle/ParticleSystem.h"
#include "renderer/MinimapRenderer.h"
#include "skin/SkinManager.h"

class GameEngine;

class QPainterGLWidget : public QWidget {
    Q_OBJECT
public:
    explicit QPainterGLWidget(QWidget* parent = nullptr);
    ~QPainterGLWidget() override;

    void setEngine(GameEngine* engine);
    void setShowMinimap(bool show) { m_showMinimap = show; }

protected:
    void paintEvent(QPaintEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    void keyReleaseEvent(QKeyEvent* event) override;

private:
    GameEngine* m_engine = nullptr;
    QPointF m_cameraPos = {3000, 3000};
    float m_viewZoom = 1.0f;
    qint64 m_lastTime = 0;

    ParticleSystem m_particles;
    MinimapRenderer m_minimap;

    struct TrailPoint { QPointF pos; float alpha; };
    QVector<TrailPoint> m_trail;
    QPointF m_lastPlayerPos;

    float m_prevMass = 10.0f;
    int m_prevCellCount = 1;
    int m_prevFoodCount = 0;
    float m_pulseAmount = 0.0f;
    SkinDef m_playerSkin;

    int m_frameCount = 0;
    float m_fps = 0.0f;
    qint64 m_fpsTimer = 0;

    bool m_eKeyDown = false;
    bool m_showMinimap = true;

    QPointF worldToScreen(float wx, float wy) const;
    void drawBall(QPainter& p, const QPointF& center, float radius, const QColor& color, const QString& name);
};

#endif // QPAINTERGLWIDGET_H
