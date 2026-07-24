#include "GridRenderer.h"
#include "Camera.h"
#include <QDebug>

static const char* gridVert = R"(
#version 330 core
layout(location = 0) in vec2 aPos;
out vec2 vClipPos;
void main() {
    vClipPos = aPos;
    gl_Position = vec4(aPos, 0.0, 1.0);
}
)";

static const char* gridFrag = R"(
#version 330 core
out vec4 fragColor;
uniform mat4 uInvVP;
uniform vec2 uGridSize;
uniform vec2 uWorldSize;
uniform float uZoom;
uniform vec2 uSafeCenter;     // 安全区中心（世界坐标）
uniform float uSafeRadius;    // 安全区半径（>0 时启用 BR 模式）
uniform float uTime;          // 用于边缘脉动

in vec2 vClipPos;

void main() {
    vec4 worldPos4 = uInvVP * vec4(vClipPos, 0.0, 1.0);
    vec2 worldPos = worldPos4.xy / worldPos4.w;

    // 边界宽度根据 zoom 自适应
    float borderWidth = max(10.0, 4.0 / max(uZoom, 0.01));

    bool outsideX = worldPos.x < 0.0 || worldPos.x > uWorldSize.x;
    bool outsideY = worldPos.y < 0.0 || worldPos.y > uWorldSize.y;

    if (outsideX || outsideY) {
        // 世界外区域：深灰色（与游戏背景区分），表示不可达区域
        float distToEdge = 0.0;
        if (worldPos.x < 0.0) distToEdge = max(distToEdge, -worldPos.x);
        if (worldPos.x > uWorldSize.x) distToEdge = max(distToEdge, worldPos.x - uWorldSize.x);
        if (worldPos.y < 0.0) distToEdge = max(distToEdge, -worldPos.y);
        if (worldPos.y > uWorldSize.y) distToEdge = max(distToEdge, worldPos.y - uWorldSize.y);
        // 距离地图边缘越远越暗，统一用深灰色 (0.06, 0.06, 0.10)
        float fade = 1.0 - smoothstep(0.0, 800.0, distToEdge);
        fragColor = vec4(0.06, 0.06, 0.10, 0.6 + 0.3 * fade);
        return;
    }

    // === 大逃杀模式：安全区与危险区 ===
    bool brActive = uSafeRadius > 0.5;
    float distToSafeCenter = length(worldPos - uSafeCenter);
    bool inDangerZone = brActive && distToSafeCenter > uSafeRadius;

    // 网格线
    vec2 grid = abs(fract(worldPos / uGridSize));
    vec2 lineDist = min(grid, 1.0 - grid);
    float line = min(lineDist.x, lineDist.y);
    float gridAlpha = 1.0 - smoothstep(0.0, 0.02, line);

    // 世界边界线（金色）
    float borderX = min(worldPos.x, uWorldSize.x - worldPos.x);
    float borderY = min(worldPos.y, uWorldSize.y - worldPos.y);
    float borderDist = min(borderX, borderY);
    float borderAlpha = 1.0 - smoothstep(0.0, borderWidth, borderDist);

    // 基础颜色：网格 + 边界
    vec4 baseColor;
    if (borderAlpha > 0.05) {
        baseColor = vec4(1.0, 0.84, 0.0, borderAlpha * 0.8);
    } else {
        baseColor = vec4(0.35, 0.35, 0.55, gridAlpha * 0.35);
    }

    if (brActive) {
        // 危险区填充：圈外区域淡红色覆盖
        if (inDangerZone) {
            float depth = clamp((distToSafeCenter - uSafeRadius) / 300.0, 0.0, 1.0);
            vec3 dangerCol = vec3(0.5, 0.05, 0.1);
            baseColor.rgb = mix(baseColor.rgb, dangerCol, 0.4 + depth * 0.3);
            baseColor.a = max(baseColor.a, 0.25 + depth * 0.15);
        }

        // 安全区圆边：双层（外圈虚线+内圈发光）
        float ringDist = abs(distToSafeCenter - uSafeRadius);
        float ringWidth = max(8.0, 3.0 / max(uZoom, 0.01));
        float ringAlpha = 1.0 - smoothstep(0.0, ringWidth, ringDist);

        // 脉动效果：宽度按 sin 波动
        float pulse = 0.7 + 0.3 * sin(uTime * 3.0);

        if (ringAlpha > 0.05) {
            vec3 ringCol = vec3(0.3, 0.85, 1.0);  // 青色
            baseColor = vec4(ringCol, ringAlpha * pulse);
        }
    }

    fragColor = baseColor;
}
)";

GridRenderer::GridRenderer() {}
GridRenderer::~GridRenderer() {
    delete m_prog;
    if (m_vao) glDeleteVertexArrays(1, &m_vao);
    if (m_vbo) glDeleteBuffers(1, &m_vbo);
}

bool GridRenderer::initialize() {
    initializeOpenGLFunctions();

    m_prog = new QOpenGLShaderProgram();
    if (!m_prog->addShaderFromSourceCode(QOpenGLShader::Vertex, gridVert)) {
        qWarning() << "Grid vert:" << m_prog->log(); delete m_prog; m_prog = nullptr; return false;
    }
    if (!m_prog->addShaderFromSourceCode(QOpenGLShader::Fragment, gridFrag)) {
        qWarning() << "Grid frag:" << m_prog->log(); delete m_prog; m_prog = nullptr; return false;
    }
    if (!m_prog->link()) {
        qWarning() << "Grid link:" << m_prog->log(); delete m_prog; m_prog = nullptr; return false;
    }

    m_uInvVP = m_prog->uniformLocation("uInvVP");
    m_uGridSize = m_prog->uniformLocation("uGridSize");
    m_uWorldSize = m_prog->uniformLocation("uWorldSize");
    m_uZoom = m_prog->uniformLocation("uZoom");
    m_uSafeCenter = m_prog->uniformLocation("uSafeCenter");
    m_uSafeRadius = m_prog->uniformLocation("uSafeRadius");
    m_uTime = m_prog->uniformLocation("uTime");

    float vertices[] = {
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
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glBindVertexArray(0);

    return true;
}

void GridRenderer::render(const Camera& camera, float safeZoneRadius,
                          float safeCenterX, float safeCenterY, float time) {
    if (!m_prog) return;

    m_prog->bind();

    QMatrix4x4 vp = camera.projection() * camera.view();
    QMatrix4x4 invVP = vp.inverted();
    m_prog->setUniformValue(m_uInvVP, invVP);
    m_prog->setUniformValue(m_uGridSize, QVector2D(100.0f, 100.0f));
    // 使用真实世界大小
    m_prog->setUniformValue(m_uWorldSize, QVector2D(camera.worldW(), camera.worldH()));
    m_prog->setUniformValue(m_uZoom, camera.zoom());
    m_prog->setUniformValue(m_uSafeCenter, QVector2D(safeCenterX, safeCenterY));
    m_prog->setUniformValue(m_uSafeRadius, safeZoneRadius);
    m_prog->setUniformValue(m_uTime, time);

    glBindVertexArray(m_vao);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);

    m_prog->release();
}
