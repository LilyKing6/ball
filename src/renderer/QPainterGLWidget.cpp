#include "QPainterGLWidget.h"
#include "engine/GameEngine.h"
#include "engine/World.h"
#include "util/Math.h"
#include "util/Config.h"
#include "util/Random.h"
#include <QPainter>
#include <QPainterPath>
#include <QTimer>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QDateTime>
#include <QDebug>

QPainterGLWidget::QPainterGLWidget(QWidget* parent) : QWidget(parent), m_particles(500) {
    setFocusPolicy(Qt::StrongFocus);
    setMouseTracking(true);
    setStyleSheet("background-color: #1a1a2e;");
    m_lastTime = QDateTime::currentMSecsSinceEpoch();
    SkinManager::instance().loadDefaults();
    m_playerSkin = SkinManager::instance().getSkin("default");
}

QPainterGLWidget::~QPainterGLWidget() {}

void QPainterGLWidget::setEngine(GameEngine* engine) {
    m_engine = engine;
    m_cameraPos = QPointF(3000, 3000);
    m_viewZoom = 1.0f;
}

QPointF QPainterGLWidget::worldToScreen(float wx, float wy) const {
    QSize sz = size();
    if (sz.isEmpty()) return QPointF();
    float scale = m_viewZoom * 3.0f;
    return QPointF(
        sz.width() / 2.0f + (wx - m_cameraPos.x()) * scale,
        sz.height() / 2.0f + (wy - m_cameraPos.y()) * scale
    );
}

void QPainterGLWidget::drawBall(QPainter& p, const QPointF& center, float radius, const QColor& color, const QString& name) {
    float displayR = radius * (1.0f + m_pulseAmount);
    if (displayR < 8) displayR = 8;

    // Glow
    p.setPen(Qt::NoPen);
    QColor glowColor = color;
    glowColor.setAlpha(40);
    p.setBrush(glowColor);
    p.drawEllipse(center, displayR * 1.3f, displayR * 1.3f);

    // Skin
    SkinManager::instance().applySkin(p, m_playerSkin, center, displayR);

    // Highlight
    QPointF highlight = center - QPointF(displayR * 0.3f, displayR * 0.3f);
    QRadialGradient hl(highlight, displayR * 0.4f);
    hl.setColorAt(0, QColor(255, 255, 255, 80));
    hl.setColorAt(1, QColor(255, 255, 255, 0));
    p.setBrush(hl);
    p.setPen(Qt::NoPen);
    p.drawEllipse(highlight, displayR * 0.35f, displayR * 0.35f);

    // Name label
    if (displayR > 25 && !name.isEmpty()) {
        int fontSize = qMax(10, (int)(displayR * 0.35f));
        QFont font("Microsoft YaHei", fontSize, QFont::Bold);
        QPainterPath textPath;
        textPath.addText(center.x(), center.y() + fontSize * 0.3f, font, name);
        QRectF textRect = textPath.boundingRect();
        textPath.translate(-textRect.width() / 2.0f, 0);
        p.setPen(QPen(QColor(0, 0, 0, 180), 2));
        p.setBrush(Qt::NoBrush);
        p.drawPath(textPath);
        p.setPen(Qt::white);
        p.setBrush(Qt::white);
        p.drawPath(textPath);
    }
}

void QPainterGLWidget::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), QColor(26, 26, 46));

    if (!m_engine) {
        p.setPen(Qt::white);
        p.setFont(QFont("Arial", 24));
        p.drawText(rect(), Qt::AlignCenter, "Loading...");
        QTimer::singleShot(50, this, SLOT(update()));
        return;
    }

    qint64 now = QDateTime::currentMSecsSinceEpoch();
    float dt = (now - m_lastTime) / 1000.0f;
    m_lastTime = now;
    if (dt < 0.001f || dt > 0.1f) dt = 0.016f;

    m_engine->update(dt);

    if (m_eKeyDown && m_engine) m_engine->ejectFromLocalPlayer();

    const World& world = m_engine->world();
    const Player* local = world.localPlayer();

    // Detect state changes for particles
    if (local) {
        float curMass = local->totalMass();
        int curCells = 0;
        for (auto& c : local->cells) if (c.alive) curCells++;
        int aliveFood = 0;
        for (auto& f : world.foods()) if (f.alive) aliveFood++;

        QPointF localScreen = worldToScreen(local->centerOfMass().x, local->centerOfMass().y);

        // Food eaten
        if (m_prevFoodCount > 0 && aliveFood < m_prevFoodCount) {
            int eaten = m_prevFoodCount - aliveFood;
            for (int i = 0; i < qMin(eaten, 3); i++) {
                QColor fc = QColor::fromHsv(randInt(0, 359), 200, 230);
                m_particles.emitBurst(localScreen, fc, 5 + randInt(0, 3));
            }
            m_pulseAmount = 0.15f;
        }

        // Split
        if (curCells > m_prevCellCount) {
            m_particles.emitRing(localScreen, local->cells[0].color, 60.0f, 12 + randInt(0, 4));
            m_playerSkin = SkinManager::instance().getSkin(
                SkinManager::instance().allSkins()[randInt(0, SkinManager::instance().allSkins().size() - 1)].id);
        }

        // Merge
        if (curCells < m_prevCellCount && curCells > 0) {
            m_particles.emitImplode(localScreen, local->cells[0].color, 8 + randInt(0, 2));
        }

        // Death
        if (curMass <= 0 && m_prevMass > 0) {
            m_particles.emitBurst(localScreen, local->cells[0].color, 20 + randInt(0, 10));
        }

        m_prevMass = curMass;
        m_prevCellCount = curCells;
        m_prevFoodCount = aliveFood;
    }

    // Pulse decay
    if (m_pulseAmount > 0.001f) {
        m_pulseAmount *= 0.9f;
    } else {
        m_pulseAmount = 0.0f;
    }

    // Camera
    if (local) {
        Vec2 com = local->centerOfMass();
        float mass = local->totalMass();
        QSize sz = size();

        m_cameraPos.setX(m_cameraPos.x() * 0.9 + com.x * 0.1);
        m_cameraPos.setY(m_cameraPos.y() * 0.9 + com.y * 0.1);

        float baseZoom = 1.0f / qSqrt(qMax(1.0f, mass) / 20.0f);
        if (local->cells.size() > 1) {
            float minX = 1e9f, maxX = -1e9f, minY = 1e9f, maxY = -1e9f;
            for (auto& c : local->cells) {
                if (!c.alive) continue;
                minX = qMin(minX, c.pos.x - c.radius());
                maxX = qMax(maxX, c.pos.x + c.radius());
                minY = qMin(minY, c.pos.y - c.radius());
                maxY = qMax(maxY, c.pos.y + c.radius());
            }
            float spreadW = maxX - minX + 100;
            float spreadH = maxY - minY + 100;
            float neededZoom = qMin(sz.width() / spreadW, sz.height() / spreadH);
            baseZoom = qMin(baseZoom, neededZoom * 0.7f);
        }
        float targetZoom = qBound(0.2f, baseZoom, 5.0f);
        m_viewZoom = m_viewZoom * 0.92 + targetZoom * 0.08;
    }

    m_particles.update(dt);

    QSize sz = size();
    float scale = m_viewZoom * 3.0f;

    // Trail update
    if (local && local->totalMass() > 0) {
        Vec2 com = local->centerOfMass();
        QPointF curScreen = worldToScreen(com.x, com.y);
        if (!m_lastPlayerPos.isNull()) {
            float dist = (curScreen - m_lastPlayerPos).manhattanLength();
            if (dist > 3.0f) {
                m_trail.prepend({curScreen, 0.6f});
                if (m_trail.size() > 8) m_trail.resize(8);
            }
        }
        m_lastPlayerPos = curScreen;
    }

    // Trail decay
    for (auto& tp : m_trail) tp.alpha *= 0.85f;
    m_trail.erase(std::remove_if(m_trail.begin(), m_trail.end(),
        [](const TrailPoint& tp) { return tp.alpha < 0.02f; }), m_trail.end());

    // Grid
    p.setPen(QColor(50, 50, 80));
    int step = 200;
    int startX = ((int)m_cameraPos.x() / step) * step - step * 3;
    int startY = ((int)m_cameraPos.y() / step) * step - step * 3;
    for (int x = startX; x <= startX + step * 7; x += step) {
        QPointF sp = worldToScreen(x, 0);
        if (sp.x() > -10 && sp.x() < sz.width() + 10)
            p.drawLine(sp.x(), 0, sp.x(), sz.height());
    }
    for (int y2 = startY; y2 <= startY + step * 7; y2 += step) {
        QPointF sp = worldToScreen(0, y2);
        if (sp.y() > -10 && sp.y() < sz.height() + 10)
            p.drawLine(0, sp.y(), sz.width(), sp.y());
    }

    // Border
    QRectF border(
        worldToScreen(0, 0).x(), worldToScreen(0, 0).y(),
        worldToScreen(6000, 6000).x() - worldToScreen(0, 0).x(),
        worldToScreen(6000, 6000).y() - worldToScreen(0, 0).y()
    );
    p.setPen(QPen(QColor(255, 215, 0), 4));
    p.setBrush(Qt::NoBrush);
    p.drawRect(border);

    // Food
    for (const auto& f : world.foods()) {
        QPointF sp = worldToScreen(f.pos.x, f.pos.y);
        float fr = Config::instance().foodRadius * scale;
        if (fr < 2) fr = 2;
        if (sp.x() > -fr && sp.x() < sz.width() + fr && sp.y() > -fr && sp.y() < sz.height() + fr) {
            p.setBrush(f.color);
            p.setPen(Qt::NoPen);
            p.drawEllipse(sp, fr, fr);
        }
    }

    // Viruses
    for (const auto& v : world.viruses()) {
        if (!v.alive) continue;
        QPointF sp = worldToScreen(v.pos.x, v.pos.y);
        float r = v.radius() * scale;
        if (r < 6) r = 6;
        if (sp.x() > -r && sp.x() < sz.width() + r && sp.y() > -r && sp.y() < sz.height() + r) {
            QRadialGradient grad(sp, r);
            grad.setColorAt(0, QColor(60, 220, 60));
            grad.setColorAt(1, QColor(0, 120, 0));
            p.setBrush(grad);
            p.setPen(QPen(QColor(0, 80, 0), 2));
            p.drawEllipse(sp, r, r);
        }
    }

    // Spores
    for (const auto& em : world.spores()) {
        if (!em.alive) continue;
        QPointF sp = worldToScreen(em.pos.x, em.pos.y);
        float r = em.radius() * scale;
        if (r < 5) r = 5;
        if (sp.x() > -r && sp.x() < sz.width() + r && sp.y() > -r && sp.y() < sz.height() + r) {
            p.setBrush(QColor(255, 180, 50));
            p.setPen(QPen(QColor(200, 140, 30), 1));
            p.drawEllipse(sp, r, r);
        }
    }

    // Big Beans
    for (const auto& bb : world.bigBeans()) {
        if (!bb.alive) continue;
        QPointF sp = worldToScreen(bb.pos.x, bb.pos.y);
        float r = bb.radius() * scale;
        if (r < 10) r = 10;
        if (sp.x() > -r && sp.x() < sz.width() + r && sp.y() > -r && sp.y() < sz.height() + r) {
            QRadialGradient grad(sp, r);
            grad.setColorAt(0, bb.color);
            grad.setColorAt(1, bb.color.darker(150));
            p.setBrush(grad);
            p.setPen(QPen(bb.color.darker(200), 2));
            p.drawEllipse(sp, r, r);
        }
    }

    // Trail rendering (before players, behind them visually)
    for (const auto& tp : m_trail) {
        if (local && local->totalMass() > 0) {
            float trailR = local->cells[0].radius() * scale * (1.0f + m_pulseAmount) * 0.8f;
            QColor tc = m_playerSkin.primaryColor;
            tc.setAlpha(static_cast<int>(tp.alpha * 60));
            p.setBrush(tc);
            p.setPen(Qt::NoPen);
            p.drawEllipse(tp.pos, trailR * tp.alpha, trailR * tp.alpha);
        }
    }

    // Players
    for (const auto& pl : world.players()) {
        for (const auto& c : pl.cells) {
            if (!c.alive) continue;
            QPointF sp = worldToScreen(c.pos.x, c.pos.y);
            float r = c.radius() * scale;
            if (sp.x() > -r && sp.x() < sz.width() + r && sp.y() > -r && sp.y() < sz.height() + r) {
                drawBall(p, sp, r, c.color, pl.name);
            }
        }
    }

    // Direction indicator
    if (local) {
        Vec2 com = local->centerOfMass();
        QPointF ballScreen = worldToScreen(com.x, com.y);
        QPointF mouseScreen = worldToScreen(local->mouseWorldPos.x, local->mouseWorldPos.y);

        QPen dashPen(QColor(255, 255, 255, 50), 1, Qt::DashLine);
        p.setPen(dashPen);
        p.drawLine(ballScreen, mouseScreen);

        p.setPen(QPen(QColor(255, 180, 50, 150), 2));
        p.setBrush(QColor(255, 180, 50, 30));
        p.drawEllipse(mouseScreen, 10, 10);
        p.setPen(QPen(QColor(255, 180, 50, 100), 1));
        p.drawEllipse(mouseScreen, 6, 6);
    }

    // Particles
    m_particles.render(p);

    // Minimap
    if (m_showMinimap)
        m_minimap.render(p, rect(), world);

    // FPS
    m_frameCount++;
    qint64 nowMs = QDateTime::currentMSecsSinceEpoch();
    if (nowMs - m_fpsTimer >= 1000) {
        m_fps = m_frameCount * 1000.0f / (nowMs - m_fpsTimer);
        m_frameCount = 0;
        m_fpsTimer = nowMs;
    }
    p.setPen(QColor(255, 255, 255, 180));
    p.setFont(QFont("Consolas", 12));
    p.drawText(sz.width() - 80, 20, QString("FPS: %1").arg((int)m_fps));

    QTimer::singleShot(16, this, SLOT(update()));
}

void QPainterGLWidget::mouseMoveEvent(QMouseEvent* e) {
    if (!m_engine || size().isEmpty()) { QWidget::mouseMoveEvent(e); return; }
    QSize sz = size();
    float scale = m_viewZoom * 3.0f;
    float wx = m_cameraPos.x() + (e->pos().x() - sz.width() / 2.0f) / scale;
    float wy = m_cameraPos.y() + (e->pos().y() - sz.height() / 2.0f) / scale;
    wx = qBound(0.0f, wx, 6000.0f);
    wy = qBound(0.0f, wy, 6000.0f);
    m_engine->setLocalPlayerMousePos({wx, wy});
    QWidget::mouseMoveEvent(e);
}

void QPainterGLWidget::keyPressEvent(QKeyEvent* e) {
    if (e->key() == Qt::Key_Space && !e->isAutoRepeat() && m_engine) m_engine->splitLocalPlayer();
    else if (e->key() == Qt::Key_E) {
        m_eKeyDown = true;
        // Eject trail particle
        if (m_engine && m_engine->world().localPlayer()) {
            Vec2 com = m_engine->world().localPlayer()->centerOfMass();
            QPointF sp = worldToScreen(com.x, com.y);
            m_particles.emitTrail(sp, QColor(255, 180, 50));
        }
    }
    QWidget::keyPressEvent(e);
}

void QPainterGLWidget::keyReleaseEvent(QKeyEvent* e) {
    if (e->key() == Qt::Key_E) m_eKeyDown = false;
    QWidget::keyReleaseEvent(e);
}
