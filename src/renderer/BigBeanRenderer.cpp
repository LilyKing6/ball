#include "BigBeanRenderer.h"
#include "Camera.h"
#include "util/Config.h"
#include <QCoreApplication>
#include <QDebug>

BigBeanRenderer::BigBeanRenderer() {}
BigBeanRenderer::~BigBeanRenderer() {
    delete m_prog;
    if (m_vao) glDeleteVertexArrays(1, &m_vao);
    if (m_vbo) glDeleteBuffers(1, &m_vbo);
    if (m_instanceVbo) glDeleteBuffers(1, &m_instanceVbo);
}

void BigBeanRenderer::setupQuad() {
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
    glBufferData(GL_ARRAY_BUFFER, MAX_INSTANCES * sizeof(BigBeanInstance), nullptr, GL_DYNAMIC_DRAW);

    int stride = sizeof(BigBeanInstance);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, stride, (void*)offsetof(BigBeanInstance, posX));
    glVertexAttribDivisor(1, 1);

    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, stride, (void*)offsetof(BigBeanInstance, colorR));
    glVertexAttribDivisor(2, 1);

    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, stride, (void*)offsetof(BigBeanInstance, radius));
    glVertexAttribDivisor(3, 1);

    glEnableVertexAttribArray(4);
    glVertexAttribPointer(4, 1, GL_FLOAT, GL_FALSE, stride, (void*)offsetof(BigBeanInstance, pulsePhase));
    glVertexAttribDivisor(4, 1);

    glBindVertexArray(0);
}

bool BigBeanRenderer::initialize() {
    initializeOpenGLFunctions();

    // Use inline shaders to avoid file dependency
    const char* vertSrc = R"(
#version 330 core
layout(location = 0) in vec2 aPos;
layout(location = 1) in vec2 aInstancePos;
layout(location = 2) in vec4 aInstanceColor;
layout(location = 3) in float aInstanceRadius;
layout(location = 4) in float aPulsePhase;

uniform mat4 uProjection;
uniform mat4 uView;
uniform float uTime;

out vec4 vColor;
out vec2 vLocalPos;
out float vPulsePhase;

void main() {
    vColor = aInstanceColor;
    vLocalPos = aPos;
    vPulsePhase = aPulsePhase;
    float pulse = 1.0 + 0.08 * sin(uTime * 3.0 + aPulsePhase);
    vec2 worldPos = aPos * aInstanceRadius * pulse + aInstancePos;
    gl_Position = uProjection * uView * vec4(worldPos, 0.0, 1.0);
}
)";

    const char* fragSrc = R"(
#version 330 core
in vec4 vColor;
in vec2 vLocalPos;
in float vPulsePhase;

uniform float uTime;

out vec4 fragColor;

void main() {
    float dist = length(vLocalPos);
    float glow = 1.0 - smoothstep(0.6, 1.0, dist);
    float core = 1.0 - smoothstep(0.0, 0.85, dist);

    vec3 glowCol = vColor.rgb * 0.5;
    vec3 coreCol = mix(vColor.rgb, vec3(1.0), 0.3);

    float alpha = glow * 0.4 + core * 0.9;
    vec3 col = mix(glowCol, coreCol, core);

    fragColor = vec4(col, alpha);
    if (fragColor.a < 0.01) discard;
}
)";

    m_prog = new QOpenGLShaderProgram();
    if (!m_prog->addShaderFromSourceCode(QOpenGLShader::Vertex, vertSrc) ||
        !m_prog->addShaderFromSourceCode(QOpenGLShader::Fragment, fragSrc) ||
        !m_prog->link()) {
        qWarning() << "BigBean shader:" << m_prog->log();
        delete m_prog; m_prog = nullptr; return false;
    }
    m_uProj = m_prog->uniformLocation("uProjection");
    m_uView = m_prog->uniformLocation("uView");
    m_uTime = m_prog->uniformLocation("uTime");

    setupQuad();
    return true;
}

void BigBeanRenderer::render(const QVector<BigBean>& beans, float time, const Camera& camera) {
    if (!m_prog) return;

    m_instances.clear();
    m_instances.reserve(qMin(beans.size(), MAX_INSTANCES));

    for (const auto& b : beans) {
        if (!b.alive) continue;
        if (m_instances.size() >= MAX_INSTANCES) break;
        BigBeanInstance bi;
        bi.posX = b.pos.x;
        bi.posY = b.pos.y;
        bi.colorR = b.color.redF();
        bi.colorG = b.color.greenF();
        bi.colorB = b.color.blueF();
        bi.colorA = 1.0f;
        bi.radius = b.radius();
        bi.pulsePhase = b.pulsePhase;
        m_instances.append(bi);
    }

    if (m_instances.isEmpty()) return;

    m_prog->bind();
    m_prog->setUniformValue(m_uProj, camera.projection());
    m_prog->setUniformValue(m_uView, camera.view());
    m_prog->setUniformValue(m_uTime, time);

    glBindVertexArray(m_vao);
    glBindBuffer(GL_ARRAY_BUFFER, m_instanceVbo);
    glBufferSubData(GL_ARRAY_BUFFER, 0, m_instances.size() * sizeof(BigBeanInstance), m_instances.data());
    glDrawArraysInstanced(GL_TRIANGLES, 0, 6, m_instances.size());
    glBindVertexArray(0);

    m_prog->release();
}
