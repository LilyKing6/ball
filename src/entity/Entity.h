#ifndef ENTITY_H
#define ENTITY_H

#include "util/Math.h"
#include "util/Config.h"

struct Entity {
    Vec2 pos;
    Vec2 vel;
    float mass = 10.0f;
    bool alive = true;

    Entity() = default;
    virtual ~Entity() = default;

    float radius() const { return Config::instance().radiusConstant * qSqrt(mass); }

    virtual void update(float dt) {
        pos += vel * dt;
    }
};

#endif // ENTITY_H
