#ifndef FOODRENDERER_H
#define FOODRENDERER_H

#include <QOpenGLFunctions_3_3_Core>
#include <QOpenGLShaderProgram>
#include <QVector>
#include "entity/Food.h"

struct FoodInstance {
    float posX, posY;
    float colorR, colorG, colorB, colorA;
    float radius;
};

class FoodRenderer : protected QOpenGLFunctions_3_3_Core {
public:
    FoodRenderer();
    ~FoodRenderer();

    bool initialize();
    void render(const QVector<Food>& foods, const class Camera& camera);

private:
    QOpenGLShaderProgram* m_prog = nullptr;
    unsigned int m_vao = 0, m_vbo = 0, m_instanceVbo = 0;
    int m_uProj = -1, m_uView = -1;
    QVector<FoodInstance> m_instances;
    static constexpr int MAX_INSTANCES = 4096;

    void setupQuad();
};

#endif // FOODRENDERER_H
