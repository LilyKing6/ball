#ifndef BIGBEANRENDERER_H
#define BIGBEANRENDERER_H

#include <QOpenGLFunctions_3_3_Core>
#include <QOpenGLShaderProgram>
#include <QVector>
#include "entity/BigBean.h"

struct BigBeanInstance {
    float posX, posY;
    float colorR, colorG, colorB, colorA;
    float radius;
    float pulsePhase;
};

class BigBeanRenderer : protected QOpenGLFunctions_3_3_Core {
public:
    BigBeanRenderer();
    ~BigBeanRenderer();

    bool initialize();
    void render(const QVector<BigBean>& beans, float time, const class Camera& camera);

private:
    QOpenGLShaderProgram* m_prog = nullptr;
    unsigned int m_vao = 0, m_vbo = 0, m_instanceVbo = 0;
    int m_uProj = -1, m_uView = -1, m_uTime = -1;
    QVector<BigBeanInstance> m_instances;
    static constexpr int MAX_INSTANCES = 256;

    void setupQuad();
};

#endif // BIGBEANRENDERER_H
