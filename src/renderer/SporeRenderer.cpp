#include "SporeRenderer.h"
#include "Camera.h"
#include <QCoreApplication>
#include <QDebug>

struct SporeInstance {
    float posX, posY;
    float colorR, colorG, colorB, colorA;
    float radius;
};

SporeRenderer::SporeRenderer() {}
SporeRenderer::~SporeRenderer() {
    delete m_prog;
    if (m_vao) glDeleteVertexArrays(1, &m_vao);
    if (m_vbo) glDeleteBuffers(1, &m_vbo);
    if (m_instanceVbo) glDeleteBuffers(1, &m_instanceVbo);
}

bool SporeRenderer::initialize() {
    initializeOpenGLFunctions();

    QString resPath = QCoreApplication::applicationDirPath() + "/resources/shaders/";
    m_prog = new QOpenGLShaderProgram();
    if (!m_prog->addShaderFromSourceFile(QOpenGLShader::Vertex, resPath + "food.vert") ||
        !m_prog->addShaderFromSourceFile(QOpenGLShader::Fragment, resPath + "food.frag") ||
        !m_prog->link()) {
        qWarning() << "Spore shader:" << m_prog->log();
        delete m_prog; m_prog = nullptr; return false;
    }
    m_uProj = m_prog->uniformLocation("uProjection");
    m_uView = m_prog->uniformLocation("uView");

    float quad[] = {
        -1.0f, -1.0f, 1.0f, -1.0f, 1.0f, 1.0f,
        -1.0f, -1.0f, 1.0f, 1.0f, -1.0f, 1.0f
    };
    glGenVertexArrays(1, &m_vao);
    glGenBuffers(1, &m_vbo);
    glBindVertexArray(m_vao);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quad), quad, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);

    glGenBuffers(1, &m_instanceVbo);
    glBindBuffer(GL_ARRAY_BUFFER, m_instanceVbo);
    glBufferData(GL_ARRAY_BUFFER, MAX_INSTANCES * sizeof(SporeInstance), nullptr, GL_DYNAMIC_DRAW);

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(SporeInstance), (void*)offsetof(SporeInstance, posX));
    glVertexAttribDivisor(1, 1);

    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, sizeof(SporeInstance), (void*)offsetof(SporeInstance, colorR));
    glVertexAttribDivisor(2, 1);

    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, sizeof(SporeInstance), (void*)offsetof(SporeInstance, radius));
    glVertexAttribDivisor(3, 1);

    glBindVertexArray(0);
    return true;
}

void SporeRenderer::render(const QVector<Spore>& masses, const Camera& camera) {
    if (!m_prog || masses.isEmpty()) return;

    QVector<SporeInstance> instances;
    for (const auto& m : masses) {
        if (!m.alive) continue;
        if (instances.size() >= MAX_INSTANCES) break;
        instances.append({m.pos.x, m.pos.y, 1.0f, 0.7f, 0.2f, 0.9f, m.radius()});
    }

    if (instances.isEmpty()) return;

    m_prog->bind();
    m_prog->setUniformValue(m_uProj, camera.projection());
    m_prog->setUniformValue(m_uView, camera.view());

    glBindVertexArray(m_vao);
    glBindBuffer(GL_ARRAY_BUFFER, m_instanceVbo);
    glBufferSubData(GL_ARRAY_BUFFER, 0, instances.size() * sizeof(SporeInstance), instances.data());
    glDrawArraysInstanced(GL_TRIANGLES, 0, 6, instances.size());
    glBindVertexArray(0);

    m_prog->release();
}
