#ifndef PARTICLE_H
#define PARTICLE_H

#include <QPointF>
#include <QColor>

struct Particle {
    QPointF pos;
    QPointF vel;
    QColor color;
    float life = 1.0f;
    float maxLife = 1.0f;
    float size = 4.0f;
    bool alive = true;
};

#endif // PARTICLE_H
