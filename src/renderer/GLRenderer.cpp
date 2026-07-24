#include "GLRenderer.h"
#include "engine/World.h"
#include "ShaderManager.h"
#include "util/Config.h"
#include <QOpenGLContext>
#include <QOpenGLFunctions_3_3_Core>
#include <QDebug>

GLRenderer::GLRenderer() {}
GLRenderer::~GLRenderer() {}

bool GLRenderer::initialize() {
    initializeOpenGLFunctions();

    glClearColor(0.1f, 0.1f, 0.18f, 1.0f);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    qDebug() << "OpenGL Version:" << QString::fromLatin1((const char*)glGetString(GL_VERSION));
    qDebug() << "GLSL Version:" << QString::fromLatin1((const char*)glGetString(GL_SHADING_LANGUAGE_VERSION));

    if (!m_gridRenderer.initialize()) { qWarning() << "GridRenderer failed"; return false; }
    if (!m_ballRenderer.initialize()) { qWarning() << "BallRenderer failed"; return false; }
    if (!m_foodRenderer.initialize()) { qWarning() << "FoodRenderer failed"; return false; }
    if (!m_virusRenderer.initialize()) { qWarning() << "VirusRenderer failed"; return false; }
    if (!m_sporeRenderer.initialize()) { qWarning() << "SporeRenderer failed"; return false; }
    if (!m_bigBeanRenderer.initialize()) { qWarning() << "BigBeanRenderer failed"; return false; }

    qDebug() << "All renderers initialized successfully";
    m_camera.setTarget({3000.0f, 3000.0f}, 1.0f);

    return true;
}

void GLRenderer::resize(int w, int h) {
    m_viewportW = w;
    m_viewportH = h;
    glViewport(0, 0, w, h);

    m_camera.setWorldSize(6000.0f, 6000.0f);
}

void GLRenderer::render(float dt, const World* world) {
    m_time += dt;
    m_camera.update(dt);
    m_camera.updateProjection(m_viewportW, m_viewportH);

    if (world) {
        // 使用 World 实际尺寸（不再硬编码 6000）
        m_camera.setWorldSize(world->width(), world->height());

        if (world->localPlayer()) {
            auto& p = *world->localPlayer();
            auto& cfg = Config::instance();
            Vec2 com = p.centerOfMass();
            float totalMass = p.totalMass();
            float targetZoom = 1.0f / qSqrt(totalMass / cfg.zoomMassDivisor);
            targetZoom = clamp(targetZoom, cfg.zoomMin, cfg.zoomMax);
            m_camera.setTarget(com, targetZoom);
        }
    }

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // GridRenderer 同时绘制：网格、边界、安全区圆/危险区染色
    if (world && world->currentMode() == GameMode::BattleRoyale) {
        m_gridRenderer.render(m_camera, world->safeZoneRadius(),
                              world->safeZoneCenter().x, world->safeZoneCenter().y,
                              m_time);
    } else {
        m_gridRenderer.render(m_camera);
    }

    if (world) {
        m_foodRenderer.render(world->foods(), m_camera);
        m_sporeRenderer.render(world->spores(), m_camera);
        m_virusRenderer.render(world->viruses(), m_camera);
        m_bigBeanRenderer.render(world->bigBeans(), m_time, m_camera);
        m_ballRenderer.render(world->players(), m_camera);
    }
}
