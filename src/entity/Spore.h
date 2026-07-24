#ifndef SPORE_H
#define SPORE_H

#include "Entity.h"

class Spore : public Entity {
public:
    Spore();
    Spore(const Vec2& pos, const Vec2& vel, float mass);

    void update(float dt) override;
    void setVelocity(const Vec2& v);
    void setDecaying(float decay);

    int ownerId = -1;
    float immunityTimer = 0.3f;

private:
    float m_decayTimer = 0.0f;
    float m_decayRate = 0.3f;
};

#endif // SPORE_H