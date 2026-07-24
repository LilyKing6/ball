#include "Spore.h"
#include "util/Config.h"

Spore::Spore() {
    mass = Config::instance().ejectMass;
}

Spore::Spore(const Vec2& pos, const Vec2& vel, float m) {
    mass = m;
    this->pos = pos;
    this->vel = vel;
}

void Spore::setVelocity(const Vec2& v) {
    vel = v;
}

void Spore::setDecaying(float decay) {
    m_decayRate = decay;
}

void Spore::update(float dt) {
    vel *= Config::instance().sporeVelocityDecay;
    Entity::update(dt);
    if (m_decayTimer > 0) {
        mass -= Config::instance().sporeDecayRate * dt;
        if (mass < 1.0f) {
            alive = false;
        }
    }
}
