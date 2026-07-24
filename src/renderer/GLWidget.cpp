#include "GLWidget.h"
#include "GLRenderer.h"
#include "engine/GameEngine.h"
#include "engine/World.h"
#include "engine/WorldSnapshot.h"
#include "network/NetworkClient.h"
#include "util/Config.h"
#include "particle/ParticleSystem.h"
#include "util/Random.h"
#include <QMouseEvent>
#include <QKeyEvent>
#include <QWheelEvent>
#include <QDateTime>
#include <QSurfaceFormat>
#include <QDebug>
#include <QPainter>
#include <QResizeEvent>
#include <QElapsedTimer>
#include <cmath>

GLWidget::GLWidget(QWidget* parent) : QOpenGLWidget(parent) {
    QSurfaceFormat fmt;
    fmt.setVersion(3, 3);
    fmt.setProfile(QSurfaceFormat::CoreProfile);
    fmt.setSamples(4);
    setFormat(fmt);

    setFocusPolicy(Qt::StrongFocus);
    setMouseTracking(true);

    m_frameCount = 0;
    m_fps = 0;
    m_fpsTimer.start();
    m_lastTime = QDateTime::currentMSecsSinceEpoch();
    m_accumulator = 0.0f;

    // 创建虚拟摇杆（不可见时由 paintGL 跳过绘制）
    m_joystick = new VirtualJoystick(this);
    m_joystick->setAttribute(Qt::WA_TransparentForMouseEvents, true);
    m_joystick->setRadius(80.0f);
    m_joystick->hide();

    qDebug() << "GLWidget created";
}

GLWidget::~GLWidget() {
    makeCurrent();
    delete m_renderer;
    doneCurrent();
}

void GLWidget::initializeGL() {
    qDebug() << "initializeGL START";
    makeCurrent();

    glClearColor(0.2f, 0.2f, 0.3f, 1.0f);
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_MULTISAMPLE);
    glEnable(GL_LINE_SMOOTH);
    glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
    glHint(GL_POLYGON_SMOOTH_HINT, GL_NICEST);

    m_renderer = new GLRenderer();
    m_renderer->initialize();

    doneCurrent();
    qDebug() << "initializeGL DONE";
}

int GLWidget::effectiveControlMode() const {
    auto& cfg = Config::instance();
    if (cfg.controlMode == 0) return 0;        // 始终鼠标游标
    if (cfg.controlMode == 1) return 1;        // 始终摇杆
    return m_cursorModeActive ? 0 : 1;         // Hybrid：根据切换状态
}

QPointF GLWidget::joystickScreenPosition() const {
    auto& cfg = Config::instance();
    int w = width(), h = height();
    switch (cfg.joystickPosition) {
    case 0: return QPointF(w * 0.20f, h * 0.80f);  // 左下
    case 1: return QPointF(w * 0.80f, h * 0.80f);  // 右下
    case 2: return QPointF(w * 0.50f, h * 0.85f);  // 中下
    case 3: return QPointF(w * cfg.joystickCustomX, h * cfg.joystickCustomY);
    default: return QPointF(w * 0.20f, h * 0.80f);
    }
}

void GLWidget::updateVirtualCursor() {
    if (!m_engine || !m_renderer) return;
    auto* local = m_engine->world().localPlayer();
    if (!local) return;

    auto& cfg = Config::instance();
    int mode = effectiveControlMode();

    if (mode == 1 && m_joystick && m_joystick->isActive()) {
        // 摇杆模式：游标 = 玩家中心 + 摇杆方向 * 摇杆世界半径
        QPointF stickVec = m_joystick->stickVector();
        float sr = m_joystick->radius();
        if (sr < 1.0f) sr = 80.0f;
        Vec2 worldDir(stickVec.x() / sr, -stickVec.y() / sr);  // Y 翻转：屏幕Y下→世界Y上
        Vec2 com = local->centerOfMass();
        Vec2 cursor = com + worldDir * cfg.joystickRadius;
        m_engine->setLocalPlayerCursor(cursor);
    } else {
        // 鼠标游标模式：游标 = 玩家中心 + clamp(鼠标方向, 摇杆半径)
        Vec2 com = local->centerOfMass();
        Vec2 toMouse = m_lastMouseWorld - com;
        float dist = toMouse.length();
        float maxR = cfg.joystickRadius;
        if (dist > maxR && dist > 0.001f) {
            toMouse = toMouse * (maxR / dist);
        }
        Vec2 cursor = com + toMouse;
        m_engine->setLocalPlayerCursor(cursor);
    }
}

void GLWidget::paintGL() {
    if (!m_engine || !m_renderer) return;

    // 计算 dt 并平滑（最近 5 帧的滑动平均）避免抖动
    qint64 now = QDateTime::currentMSecsSinceEpoch();
    if (m_lastTime == 0) m_lastTime = now;
    float rawDt = (now - m_lastTime) / 1000.0f;
    m_lastTime = now;
    if (rawDt < 0.001f) rawDt = 0.001f;
    if (rawDt > 0.05f) rawDt = 0.05f;
    static float smoothedDt = 0.016f;
    smoothedDt = smoothedDt * 0.85f + rawDt * 0.15f;
    float dt = smoothedDt;

    // 网络模式：在最新 snapshot 与上一帧 snapshot 之间插值，平滑 30Hz 更新
    if (m_engine->networkMode() && m_hasPendingSnap) {
        if (m_prevSnapRecvMs > 0 && m_curSnapRecvMs > m_prevSnapRecvMs) {
            qint64 interval = m_curSnapRecvMs - m_prevSnapRecvMs;
            float alpha = 0.0f;
            if (interval > 0) {
                alpha = static_cast<float>(now - m_curSnapRecvMs) / static_cast<float>(interval);
                alpha = qBound(0.0f, alpha, 1.0f);
            }
            WorldSnapshot interp = WorldSnapshot::lerp(m_prevSnap, m_curSnap, alpha);
            m_engine->applyNetworkSnapshot(interp);
        } else if (m_engine) {
            m_engine->applyNetworkSnapshot(m_curSnap);
        }
    }

    // 每帧更新虚拟游标（使本地玩家持续移动到游标位置）
    updateVirtualCursor();

    auto& cfg = Config::instance();
    if (!m_engine->networkMode()) {
        m_accumulator += rawDt;
        int maxSteps = 5;
        while (m_accumulator >= cfg.fixedDt && maxSteps-- > 0) {
            m_engine->update(cfg.fixedDt);
            m_accumulator -= cfg.fixedDt;
        }
        if (m_accumulator < 0) m_accumulator = 0;
    }

    m_renderer->render(dt, &m_engine->world());

    m_frameCount++;
    if (m_fpsTimer.elapsed() >= 1000) {
        m_fps = m_frameCount;
        m_frameCount = 0;
        m_fpsTimer.restart();
    }

    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.setRenderHint(QPainter::SmoothPixmapTransform);

    const World& world = m_engine->world();
    const Player* local = world.localPlayer();

    // 绘制游标 / 摇杆覆盖层
    drawControlOverlay(p);

    // FPS text
    p.setPen(QColor(255, 255, 255, 180));
    p.setFont(QFont("Consolas", 12));
    p.drawText(width() - 80, 20, QString("FPS: %1").arg((int)m_fps));

    // 危险区全屏 vignette
    if (local && world.currentMode() == GameMode::BattleRoyale && local->totalMass() > 0) {
        Vec2 com = local->centerOfMass();
        float dist = (com - world.safeZoneCenter()).length();
        if (dist > world.safeZoneRadius()) {
            float ratio = qBound(0.0f, (dist - world.safeZoneRadius()) / 400.0f, 1.0f);
            int maxAlpha = static_cast<int>(ratio * 140.0f);
            if (local->shieldCount > 0) maxAlpha = qMin(maxAlpha, 60);

            QRadialGradient grad(QPointF(width() / 2.0f, height() / 2.0f),
                                 qMax(width(), height()) * 0.7f,
                                 QPointF(width() / 2.0f, height() / 2.0f));
            grad.setColorAt(0.0, QColor(255, 30, 30, 0));
            grad.setColorAt(0.6, QColor(220, 30, 30, maxAlpha / 3));
            grad.setColorAt(1.0, QColor(180, 0, 0, maxAlpha));
            p.setPen(Qt::NoPen);
            p.setBrush(grad);
            p.drawRect(rect());
        }
    }

    if (m_showMinimap) {
        m_minimapRenderer.render(p, rect(), m_engine->world());
    }

    // 网络模式断线遮罩
    if (m_reconnecting) {
        p.fillRect(rect(), QColor(0, 0, 0, 150));
        p.setPen(Qt::white);
        p.setFont(QFont("Microsoft YaHei", 24, QFont::Bold));
        QString text = m_reconnectReason.isEmpty()
            ? QStringLiteral("重连中...")
            : QStringLiteral("重连中... %1").arg(m_reconnectReason);
        p.drawText(rect(), Qt::AlignCenter, text);
    }

    update();
}

void GLWidget::drawControlOverlay(QPainter& p) {
    if (!m_engine || !m_renderer) return;
    auto* local = m_engine->world().localPlayer();
    if (!local || local->totalMass() <= 0) return;

    auto& cfg = Config::instance();
    int mode = effectiveControlMode();
    Vec2 com = local->centerOfMass();
    QPointF ballScreen = m_renderer->camera().worldToScreen(com, size());

    if (mode == 0) {
        // 鼠标游标模式：游标位置 + 摇杆活动范围圈
        QPointF cursorScreen = m_renderer->camera().worldToScreen(local->virtualCursor, size());

        // 摇杆活动范围圈（虚线，淡白）：让玩家看清游标最大偏移半径
        // 屏幕半径 ≈ joystickRadius * 相机zoom
        float zoom = m_renderer->camera().zoom();
        float screenRadius = cfg.joystickRadius * zoom;
        QPen rangePen(QColor(255, 255, 255, 40), 1, Qt::DashLine);
        p.setPen(rangePen);
        p.setBrush(Qt::NoBrush);
        p.drawEllipse(ballScreen, screenRadius, screenRadius);

        // 虚线连接球心和游标
        QPen dashPen(QColor(255, 255, 255, 80), 1, Qt::DashLine);
        p.setPen(dashPen);
        p.drawLine(ballScreen, cursorScreen);

        // 游标点
        p.setPen(QPen(QColor(255, 200, 60, 220), 2));
        p.setBrush(QColor(255, 200, 60, 60));
        p.drawEllipse(cursorScreen, 9, 9);
        p.setPen(QPen(QColor(255, 230, 100, 180), 1));
        p.drawEllipse(cursorScreen, 5, 5);
    } else {
        // 摇杆模式：屏幕固定位置绘制摇杆，由 VirtualJoystick widget 自绘
        if (!m_joystick) return;
        QPointF basePos = joystickScreenPosition();
        float r = m_joystick->radius();
        m_joystick->setGeometry(int(basePos.x() - r), int(basePos.y() - r),
                                int(r * 2), int(r * 2));
        m_joystick->setBasePosition(QPointF(r, r));
        m_joystick->raise();
        m_joystick->show();

        // 游标可视化（与游标模式一致）
        QPointF cursorScreen = m_renderer->camera().worldToScreen(local->virtualCursor, size());
        float zoom = m_renderer->camera().zoom();
        float screenRadius = cfg.joystickRadius * zoom;
        QPen rangePen(QColor(255, 255, 255, 30), 1, Qt::DashLine);
        p.setPen(rangePen);
        p.setBrush(Qt::NoBrush);
        p.drawEllipse(ballScreen, screenRadius, screenRadius);

        QPen dashPen(QColor(255, 255, 255, 60), 1, Qt::DashLine);
        p.setPen(dashPen);
        p.drawLine(ballScreen, cursorScreen);
        p.setPen(QPen(QColor(255, 215, 0, 200), 2));
        p.setBrush(QColor(255, 215, 0, 60));
        p.drawEllipse(cursorScreen, 7, 7);
    }
}

void GLWidget::resizeGL(int w, int h) {
    makeCurrent();
    glViewport(0, 0, w, h);
    if (m_renderer) m_renderer->resize(w, h);
    doneCurrent();
}

void GLWidget::mouseMoveEvent(QMouseEvent* e) {
    if (!m_engine || !m_renderer || size().isEmpty()) {
        QOpenGLWidget::mouseMoveEvent(e);
        return;
    }

    m_lastMouseScreen = e->pos();
    m_lastMouseWorld = m_renderer->camera().screenToWorld(e->pos(), size());

    // 摇杆模式：拖动时驱动摇杆
    int mode = effectiveControlMode();
    if (mode == 1 && m_joystick && m_joystick->isActive()) {
        // 转换鼠标位置到摇杆 widget 内的坐标
        QPointF localPos = e->pos() - m_joystick->geometry().topLeft();
        m_joystick->handleMove(localPos);
    }

    QOpenGLWidget::mouseMoveEvent(e);
}

void GLWidget::mousePressEvent(QMouseEvent* e) {
    if (e->button() == Qt::LeftButton) {
        int mode = effectiveControlMode();
        if (mode == 1 && m_joystick) {
            auto& cfg = Config::instance();
            QPointF basePos = joystickScreenPosition();

            // 浮动模式：摇杆中心移到鼠标按下位置
            if (!cfg.joystickFixed) {
                basePos = e->pos();
            }
            float r = m_joystick->radius();
            m_joystick->setGeometry(int(basePos.x() - r), int(basePos.y() - r),
                                    int(r * 2), int(r * 2));
            m_joystick->setBasePosition(QPointF(r, r));

            QPointF localPos = e->pos() - m_joystick->geometry().topLeft();
            m_joystick->handlePress(localPos);
            m_joystick->show();
        }
    }
    QOpenGLWidget::mousePressEvent(e);
}

void GLWidget::mouseReleaseEvent(QMouseEvent* e) {
    if (e->button() == Qt::LeftButton && m_joystick && m_joystick->isActive()) {
        m_joystick->handleRelease();
        // 释放后游标归位到玩家中心
        if (m_engine && m_engine->world().localPlayer()) {
            m_engine->setLocalPlayerCursor(m_engine->world().localPlayer()->centerOfMass());
        }
    }
    QOpenGLWidget::mouseReleaseEvent(e);
}

void GLWidget::keyPressEvent(QKeyEvent* e) {
    auto& cfg = Config::instance();

    if (e->key() == Qt::Key_Space && !e->isAutoRepeat() && m_engine) {
        m_engine->splitLocalPlayer();
    } else if (e->key() == Qt::Key_E && m_engine) {
        m_engine->ejectFromLocalPlayer();
    } else if (e->key() == Qt::Key_F3) {
        emit toggleDebugPanel();
    } else if (cfg.controlMode == 2 && e->key() == cfg.controlSwitchKey && !e->isAutoRepeat()) {
        // Hybrid 模式：切换游标/摇杆
        m_cursorModeActive = !m_cursorModeActive;
        if (m_cursorModeActive && m_joystick) m_joystick->hide();
    }
    QOpenGLWidget::keyPressEvent(e);
}

void GLWidget::keyReleaseEvent(QKeyEvent* e) {
    QOpenGLWidget::keyReleaseEvent(e);
}

void GLWidget::wheelEvent(QWheelEvent* e) {
    QOpenGLWidget::wheelEvent(e);
}

// ============================================================================
// 网络模式
// ============================================================================

void GLWidget::onSnapshotReceived(const WorldSnapshot& snap, qint64 recvMs) {
    // 双缓冲：保留最近两帧 snapshot 用于插值
    if (m_hasPendingSnap) {
        m_prevSnap = m_curSnap;
        m_prevSnapRecvMs = m_curSnapRecvMs;
    }
    m_curSnap = snap;
    m_curSnapRecvMs = recvMs;
    m_hasPendingSnap = true;
    update();
}

void GLWidget::startNetworkInputLoop() {
    if (!m_inputTimer) {
        m_inputTimer = new QTimer(this);
        connect(m_inputTimer, &QTimer::timeout, this, [this]() {
            if (m_networkClient && m_networkClient->isConnected() && m_engine) {
                m_networkClient->sendInput(m_engine->pendingInput());
                m_engine->clearPendingInputFlags();
            }
        });
    }
    m_inputTimer->start(33);  // 30Hz
}

void GLWidget::stopNetworkInputLoop() {
    if (m_inputTimer) m_inputTimer->stop();
}
