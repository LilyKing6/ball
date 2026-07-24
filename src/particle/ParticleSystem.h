#ifndef PARTICLESYSTEM_H
#define PARTICLESYSTEM_H

#include "Particle.h"
#include <QVector>
#include <QPainter>

class ParticleSystem {
public:
    ParticleSystem(int maxParticles = 500);

    void emitBurst(const QPointF& pos, const QColor& color, int count, float speed = 200.0f);
    void emitRing(const QPointF& pos, const QColor& color, float radius, int count);
    void emitTrail(const QPointF& pos, const QColor& color);
    void emitImplode(const QPointF& pos, const QColor& color, int count);

    void update(float dt);
    void render(QPainter& painter);

private:
    QVector<Particle> m_particles;
    int m_maxParticles;
    int m_activeCount = 0;
};

#endif // PARTICLESYSTEM_H
