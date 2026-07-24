#ifndef GLRENDERER_H
#define GLRENDERER_H

#include "Camera.h"
#include "GridRenderer.h"
#include "BallRenderer.h"
#include "FoodRenderer.h"
#include "VirusRenderer.h"
#include "SporeRenderer.h"
#include "BigBeanRenderer.h"
#include <QOpenGLFunctions_3_3_Core>

class World;

class GLRenderer : protected QOpenGLFunctions_3_3_Core {
public:
    GLRenderer();
    ~GLRenderer();

    bool initialize();
    void render(float dt, const World* world = nullptr);

    Camera& camera() { return m_camera; }
    const Camera& camera() const { return m_camera; }

    void resize(int w, int h);

private:
    Camera m_camera;
    GridRenderer m_gridRenderer;
    BallRenderer m_ballRenderer;
    FoodRenderer m_foodRenderer;
    VirusRenderer m_virusRenderer;
    SporeRenderer m_sporeRenderer;
    BigBeanRenderer m_bigBeanRenderer;
    int m_viewportW = 1280, m_viewportH = 720;
    float m_time = 0.0f;
};

#endif // GLRENDERER_H
