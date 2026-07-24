#ifndef VIRUSRENDERER_H
#define VIRUSRENDERER_H

#include <QOpenGLFunctions_3_3_Core>
#include <QOpenGLShaderProgram>
#include <QVector>
#include "entity/Virus.h"

class VirusRenderer : protected QOpenGLFunctions_3_3_Core {
public:
    VirusRenderer();
    ~VirusRenderer();

    bool initialize();
    void render(const QVector<Virus>& viruses, const class Camera& camera);

private:
    QOpenGLShaderProgram* m_prog = nullptr;
    unsigned int m_vao = 0, m_vbo = 0, m_instanceVbo = 0;
    int m_uProj = -1, m_uView = -1;
    static constexpr int MAX_INSTANCES = 64;
};

#endif // VIRUSRENDERER_H