#ifndef VIRUS_H
#define VIRUS_H

#include "Entity.h"

class Virus : public Entity {
public:
    Virus();
    Virus(const Vec2& pos);

    float getMass() const { return m_virusMass; }
    void setTriggered() { m_triggered = true; }
    bool isTriggered() const { return m_triggered; }

    float respawnTimer = 10.0f;

private:
    float m_virusMass = 100.0f;
    bool m_triggered = false;
};

#endif // VIRUS_H