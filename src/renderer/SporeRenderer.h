#ifndef SPORERENDERER_H
#define SPORERENDERER_H

#include <QOpenGLFunctions_3_3_Core>
#include <QOpenGLShaderProgram>
#include <QVector>
#include "entity/Spore.h"

class SporeRenderer : protected QOpenGLFunctions_3_3_Core {
public:
    SporeRenderer();
    ~SporeRenderer();

    bool initialize();
    void render(const QVector<Spore>& masses, const class Camera& camera);

private:
    QOpenGLShaderProgram* m_prog = nullptr;
    unsigned int m_vao = 0, m_vbo = 0, m_instanceVbo = 0;
    int m_uProj = -1, m_uView = -1;
    static constexpr int MAX_INSTANCES = 128;
};

#endif // SPORERENDERER_H