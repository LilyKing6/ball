#ifndef AICONTROLLER_H
#define AICONTROLLER_H

#include "Behaviors.h"
#include "util/Math.h"

class Player;
class World;

class AIController {
public:
    AIController();
    explicit AIController(AIDifficulty difficulty);

    void update(Player& self, float dt, const World& world);

private:
    AIState m_state = AIState::Wander;
    AIConfig m_config;
    float m_reactionTimer = 0.0f;

    Vec2 m_targetPos;
    int m_targetFoodIdx = -1;
    int m_targetPlayerIdx = -1;
    int m_threatPlayerIdx = -1;
    Vec2 m_nearestVirusPos;
    bool m_virusNearby = false;

    void scanEnvironment(const Player& self, const World& world);
    void evaluateState(const Player& self);

    void wander(Player& self);
    void seekFood(Player& self, const World& world);
    void chaseSmaller(Player& self, const World& world);
    void fleeBigger(Player& self, const World& world);
    void splitKill(Player& self, const World& world);

    void moveToward(Player& self, const Vec2& target);
    void moveAwayFrom(Player& self, const Vec2& threat);
};

#endif // AICONTROLLER_H
