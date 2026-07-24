#ifndef BALLRENDERER_H
#define BALLRENDERER_H

#include <QOpenGLFunctions_3_3_Core>
#include <QOpenGLShaderProgram>
#include <QVector>
#include "entity/Player.h"

struct BallInstance {
    Vec2 pos;
    float r, alpha;
    float colorR, colorG, colorB;
};

class BallRenderer : protected QOpenGLFunctions_3_3_Core {
public:
    BallRenderer();
    ~BallRenderer();

    bool initialize();
    void render(const QVector<Player>& players, const class Camera& camera);

private:
    QOpenGLShaderProgram* m_prog = nullptr;
    unsigned int m_vao = 0, m_vbo = 0, m_instanceVbo = 0;
    int m_uProj = -1, m_uView = -1;
    QVector<BallInstance> m_instances;
    static constexpr int MAX_INSTANCES = 256;

    void setupQuad();
};

#endif // BALLRENDERER_H
