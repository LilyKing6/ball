#ifndef GRIDRENDERER_H
#define GRIDRENDERER_H

#include <QOpenGLFunctions_3_3_Core>
#include <QOpenGLShaderProgram>

class Camera;

class GridRenderer : protected QOpenGLFunctions_3_3_Core {
public:
    GridRenderer();
    ~GridRenderer();

    bool initialize();
    void render(const Camera& camera, float safeZoneRadius = 0.0f,
                float safeCenterX = 0.0f, float safeCenterY = 0.0f,
                float time = 0.0f);

private:
    QOpenGLShaderProgram* m_prog = nullptr;
    unsigned int m_vao = 0, m_vbo = 0;
    int m_uInvVP = -1, m_uGridSize = -1, m_uWorldSize = -1, m_uZoom = -1;
    int m_uSafeCenter = -1, m_uSafeRadius = -1, m_uTime = -1;
};

#endif // GRIDRENDERER_H
