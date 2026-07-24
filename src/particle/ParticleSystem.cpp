#include "ParticleSystem.h"
#include "util/Random.h"
#include <QtMath>

ParticleSystem::ParticleSystem(int maxParticles) : m_maxParticles(maxParticles) {
    m_particles.resize(maxParticles);
}

void ParticleSystem::emitBurst(const QPointF& pos, const QColor& color, int count, float speed) {
    for (int i = 0; i < count; i++) {
        if (m_activeCount >= m_maxParticles) break;
        auto& p = m_particles[m_activeCount++];
        p.pos = pos;
        float angle = randFloat(0, M_PI * 2);
        float spd = randFloat(speed * 0.3f, speed);
        p.vel = QPointF(cos(angle) * spd, sin(angle) * spd);
        p.color = color;
        p.life = 1.0f;
        p.maxLife = randFloat(0.3f, 0.8f);
        p.size = randFloat(2.0f, 6.0f);
        p.alive = true;
    }
}

void ParticleSystem::emitRing(const QPointF& pos, const QColor& color, float radius, int count) {
    for (int i = 0; i < count; i++) {
        if (m_activeCount >= m_maxParticles) break;
        auto& p = m_particles[m_activeCount++];
        float angle = (float)i / count * M_PI * 2;
        p.pos = QPointF(pos.x() + cos(angle) * radius, pos.y() + sin(angle) * radius);
        p.vel = QPointF(cos(angle) * 150.0f, sin(angle) * 150.0f);
        p.color = color;
        p.life = 1.0f;
        p.maxLife = 0.5f;
        p.size = randFloat(3.0f, 7.0f);
        p.alive = true;
    }
}

void ParticleSystem::emitTrail(const QPointF& pos, const QColor& color) {
    if (m_activeCount >= m_maxParticles) return;
    auto& p = m_particles[m_activeCount++];
    p.pos = pos;
    p.vel = QPointF(randFloat(-30, 30), randFloat(-30, 30));
    p.color = color;
    p.life = 1.0f;
    p.maxLife = 0.3f;
    p.size = randFloat(2.0f, 4.0f);
    p.alive = true;
}

void ParticleSystem::emitImplode(const QPointF& pos, const QColor& color, int count) {
    for (int i = 0; i < count; i++) {
        if (m_activeCount >= m_maxParticles) break;
        auto& p = m_particles[m_activeCount++];
        float angle = randFloat(0, M_PI * 2);
        float dist = randFloat(30, 100);
        p.pos = QPointF(pos.x() + cos(angle) * dist, pos.y() + sin(angle) * dist);
        p.vel = (pos - p.pos) * 2.0f;
        p.color = color;
        p.life = 1.0f;
        p.maxLife = 0.6f;
        p.size = randFloat(3.0f, 6.0f);
        p.alive = true;
    }
}

void ParticleSystem::update(float dt) {
    int writeIdx = 0;
    for (int i = 0; i < m_activeCount; i++) {
        auto& p = m_particles[i];
        if (!p.alive) continue;

        p.pos += p.vel * dt;
        p.vel *= 0.95f;
        p.life -= dt / p.maxLife;
        p.size *= 0.995f;

        if (p.life <= 0) {
            p.alive = false;
        } else {
            if (writeIdx != i) m_particles[writeIdx] = p;
            writeIdx++;
        }
    }
    m_activeCount = writeIdx;
}

void ParticleSystem::render(QPainter& painter) {
    painter.setPen(Qt::NoPen);
    for (int i = 0; i < m_activeCount; i++) {
        auto& p = m_particles[i];
        if (!p.alive) continue;
        QColor c = p.color;
        c.setAlphaF(p.life * 0.8f);
        painter.setBrush(c);
        painter.drawEllipse(p.pos, p.size, p.size);
    }
}
