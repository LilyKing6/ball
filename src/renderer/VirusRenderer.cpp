#include "VirusRenderer.h"
#include "Camera.h"
#include <QCoreApplication>
#include <QDebug>

struct VirusInstance {
    float posX, posY;
    float radius;
};

VirusRenderer::VirusRenderer() {}
VirusRenderer::~VirusRenderer() {
    delete m_prog;
    if (m_vao) glDeleteVertexArrays(1, &m_vao);
    if (m_vbo) glDeleteBuffers(1, &m_vbo);
    if (m_instanceVbo) glDeleteBuffers(1, &m_instanceVbo);
}

bool VirusRenderer::initialize() {
    initializeOpenGLFunctions();

    QString resPath = QCoreApplication::applicationDirPath() + "/resources/shaders/";
    m_prog = new QOpenGLShaderProgram();
    if (!m_prog->addShaderFromSourceFile(QOpenGLShader::Vertex, resPath + "virus.vert") ||
        !m_prog->addShaderFromSourceFile(QOpenGLShader::Fragment, resPath + "virus.frag") ||
        !m_prog->link()) {
        qWarning() << "Virus shader:" << m_prog->log();
        delete m_prog; m_prog = nullptr; return false;
    }
    m_uProj = m_prog->uniformLocation("uProjection");
    m_uView = m_prog->uniformLocation("uView");

    float quad[] = {
        -1.0f, -1.0f,
         1.0f, -1.0f,
         1.0f,  1.0f,
        -1.0f, -1.0f,
         1.0f,  1.0f,
        -1.0f,  1.0f
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
    glBufferData(GL_ARRAY_BUFFER, MAX_INSTANCES * sizeof(VirusInstance), nullptr, GL_DYNAMIC_DRAW);

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(VirusInstance), (void*)offsetof(VirusInstance, posX));
    glVertexAttribDivisor(1, 1);

    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, sizeof(VirusInstance), (void*)offsetof(VirusInstance, radius));
    glVertexAttribDivisor(2, 1);

    glBindVertexArray(0);
    return true;
}

void VirusRenderer::render(const QVector<Virus>& viruses, const Camera& camera) {
    if (!m_prog || viruses.isEmpty()) return;

    QVector<VirusInstance> instances;
    for (const auto& v : viruses) {
        if (!v.alive) continue;
        if (instances.size() >= MAX_INSTANCES) break;
        instances.append({v.pos.x, v.pos.y, v.radius()});
    }

    if (instances.isEmpty()) return;

    m_prog->bind();
    m_prog->setUniformValue(m_uProj, camera.projection());
    m_prog->setUniformValue(m_uView, camera.view());

    glBindVertexArray(m_vao);
    glBindBuffer(GL_ARRAY_BUFFER, m_instanceVbo);
    glBufferSubData(GL_ARRAY_BUFFER, 0, instances.size() * sizeof(VirusInstance), instances.data());
    glDrawArraysInstanced(GL_TRIANGLES, 0, 6, instances.size());
    glBindVertexArray(0);

    m_prog->release();
}