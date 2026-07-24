#ifndef GLWIDGET_H
#define GLWIDGET_H

#include <QOpenGLWidget>
#include <QElapsedTimer>
#include <QPointF>
#include <QTimer>
#include "renderer/MinimapRenderer.h"
#include "ui/VirtualJoystick.h"
#include "engine/WorldSnapshot.h"
#include "util/Math.h"

class GLRenderer;
class GameEngine;
class NetworkClient;

class GLWidget : public QOpenGLWidget {
    Q_OBJECT
public:
    explicit GLWidget(QWidget* parent = nullptr);
    ~GLWidget() override;

    GLRenderer* renderer() const { return m_renderer; }
    void setEngine(GameEngine* engine) { m_engine = engine; }
    void setShowMinimap(bool v) { m_showMinimap = v; update(); }

    // 网络模式
    void setNetworkClient(NetworkClient* c) { m_networkClient = c; }
    NetworkClient* networkClient() const { return m_networkClient; }
    void onSnapshotReceived(const WorldSnapshot& snap, qint64 recvMs);
    void startNetworkInputLoop();
    void stopNetworkInputLoop();
    void showReconnectOverlay(const QString& reason) { m_reconnectReason = reason; m_reconnecting = true; update(); }
    void hideReconnectOverlay() { m_reconnecting = false; m_reconnectReason.clear(); update(); }

protected:
    void initializeGL() override;
    void paintGL() override;
    void resizeGL(int w, int h) override;

    void mouseMoveEvent(QMouseEvent* e) override;
    void mousePressEvent(QMouseEvent* e) override;
    void mouseReleaseEvent(QMouseEvent* e) override;
    void keyPressEvent(QKeyEvent* e) override;
    void keyReleaseEvent(QKeyEvent* e) override;
    void wheelEvent(QWheelEvent* e) override;

signals:
    void toggleDebugPanel();

private:
    GLRenderer* m_renderer = nullptr;
    GameEngine* m_engine = nullptr;
    qint64 m_lastTime = 0;
    float m_accumulator = 0.0f;
    bool m_showMinimap = true;
    MinimapRenderer m_minimapRenderer;

    // 虚拟游标/摇杆系统
    VirtualJoystick* m_joystick = nullptr;
    bool m_cursorModeActive = true;  // Hybrid 模式下当前是否游标模式
    QPointF m_lastMouseScreen;        // 最近鼠标位置（屏幕坐标）
    Vec2 m_lastMouseWorld = {0, 0};  // 最近鼠标位置（世界坐标）

    int m_frameCount = 0;
    float m_fps = 0;
    QElapsedTimer m_fpsTimer;

    // 网络模式
    NetworkClient* m_networkClient = nullptr;
    QTimer* m_inputTimer = nullptr;
    qint64 m_lastSnapshotMs = 0;
    bool m_hasPendingSnap = false;
    bool m_reconnecting = false;
    QString m_reconnectReason;

    // 当前生效的控制模式（综合 Config.controlMode + m_cursorModeActive）
    int effectiveControlMode() const;

    // 根据当前模式更新玩家的虚拟游标位置
    void updateVirtualCursor();

    // 计算摇杆的屏幕位置（根据 Config.joystickPosition）
    QPointF joystickScreenPosition() const;

    // 渲染虚拟游标/摇杆 UI 覆盖层
    void drawControlOverlay(QPainter& p);
};

#endif // GLWIDGET_H
