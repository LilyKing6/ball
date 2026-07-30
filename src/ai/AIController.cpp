#include "AIController.h"
#include "engine/World.h"
#include "entity/Player.h"
#include "util/Random.h"
#include <QtMath>

AIController::AIController() {
    m_config = AIConfig::forDifficulty(AIDifficulty::Normal);
    m_targetPos = {randFloat(0, 6000), randFloat(0, 6000)};
}

AIController::AIController(AIDifficulty difficulty) {
    m_config = AIConfig::forDifficulty(difficulty);
    m_targetPos = {randFloat(0, 6000), randFloat(0, 6000)};
}

void AIController::update(Player& self, float dt, const World& world) {
    m_reactionTimer -= dt;

    if (m_reactionTimer <= 0.0f) {
        scanEnvironment(self, world);
        evaluateState(self);
        m_reactionTimer = m_config.reactionDelay;
    }

    switch (m_state) {
    case AIState::Wander:       wander(self);        break;
    case AIState::SeekFood:     seekFood(self, world); break;
    case AIState::ChaseSmaller: chaseSmaller(self, world); break;
    case AIState::FleeBigger:   fleeBigger(self, world); break;
    case AIState::SplitKill:    splitKill(self, world);  break;
    }
}

void AIController::scanEnvironment(const Player& self, const World& world) {
    Vec2 com = self.centerOfMass();
    float totalM = self.totalMass();
    float radius = m_config.perceptionRadius;

    m_targetFoodIdx = -1;
    m_targetPlayerIdx = -1;
    m_threatPlayerIdx = -1;
    m_virusNearby = false;
    m_virusDanger = 0.0f;
    m_virusAvoidMul = 1.0f;
    m_nearestVirusType = VirusType::Normal;

    float closestFoodDist = radius;
    float closestPreyDist = radius;
    float closestThreatDist = radius;

    const auto& foods = world.foods();
    for (int i = 0; i < foods.size(); i++) {
        if (!foods[i].alive) continue;
        float d = (foods[i].pos - com).length();
        if (d < closestFoodDist) {
            closestFoodDist = d;
            m_targetFoodIdx = i;
        }
    }

    const auto& players = world.players();
    for (int i = 0; i < players.size(); i++) {
        if (&players[i] == &self) continue;
        float otherMass = players[i].totalMass();
        if (otherMass <= 0) continue;
        Vec2 otherCom = players[i].centerOfMass();
        float d = (otherCom - com).length();
        if (d > radius) continue;

        float ratio = totalM / otherMass;
        if (ratio > 1.1f) {
            if (d < closestPreyDist) {
                closestPreyDist = d;
                m_targetPlayerIdx = i;
            }
        } else {
            if (d < closestThreatDist) {
                closestThreatDist = d;
                m_threatPlayerIdx = i;
            }
        }
    }

    const auto& viruses = world.viruses();
    float closestVirusDist = radius * 0.5f;
    for (const auto& v : viruses) {
        if (!v.alive) continue;
        float d = (v.pos - com).length();
        if (d < closestVirusDist) {
            closestVirusDist = d;
            m_nearestVirusPos = v.pos;
            m_virusNearby = true;
            m_nearestVirusType = v.type();
            // 按类型设置危险度和躲避乘数
            switch (v.type()) {
                case VirusType::Exploder:
                    m_virusDanger = 1.0f;
                    m_virusAvoidMul = 2.0f;
                    break;
                case VirusType::Poison:
                    m_virusDanger = 0.7f;
                    m_virusAvoidMul = 1.5f;
                    break;
                case VirusType::Big:
                    m_virusDanger = 0.5f;
                    m_virusAvoidMul = 1.3f;
                    break;
                default:
                    m_virusDanger = 0.3f;
                    m_virusAvoidMul = 1.0f;
                    break;
            }
        }
    }
}

void AIController::evaluateState(const Player& self) {
    if (m_threatPlayerIdx >= 0) {
        m_state = AIState::FleeBigger;
        return;
    }

    if (m_config.canSplit && m_targetPlayerIdx >= 0 && self.canSplit()) {
        if (randFloat(0, 1) < m_config.splitSkill) {
            m_state = AIState::SplitKill;
            return;
        }
    }

    if (m_targetPlayerIdx >= 0) {
        m_state = AIState::ChaseSmaller;
        return;
    }

    if (m_targetFoodIdx >= 0) {
        m_state = AIState::SeekFood;
        return;
    }

    m_state = AIState::Wander;
}

void AIController::moveToward(Player& self, const Vec2& target) {
    Vec2 com = self.centerOfMass();
    Vec2 dir = (target - com).normalized();
    if (dir.lengthSq() < 0.001f) dir = {1.0f, 0.0f};

    if (m_virusNearby) {
        Vec2 awayFromVirus = (com - m_nearestVirusPos).normalized();
        // 按类型危险度加权(0.3~1.0)
        float w = 0.3f + m_virusDanger * 0.7f;
        dir = (dir + awayFromVirus * w).normalized();
    }

    float totalM = qMax(self.totalMass(), 10.0f);
    float offset = 8.0f * qSqrt(totalM) + 100.0f;
    self.mouseWorldPos = com + dir * offset * m_virusAvoidMul;
}

void AIController::moveAwayFrom(Player& self, const Vec2& threat) {
    Vec2 com = self.centerOfMass();
    Vec2 dir = (com - threat).normalized();
    if (dir.lengthSq() < 0.001f) dir = {1.0f, 0.0f};

    if (m_virusNearby) {
        Vec2 awayFromVirus = (com - m_nearestVirusPos).normalized();
        float w = 0.3f + m_virusDanger * 0.7f;
        dir = (dir + awayFromVirus * w).normalized();
    }

    float totalM = qMax(self.totalMass(), 10.0f);
    float offset = 8.0f * qSqrt(totalM) + 100.0f;
    self.mouseWorldPos = com + dir * offset * m_virusAvoidMul;
}

void AIController::wander(Player& self) {
    Vec2 com = self.centerOfMass();
    float dist = (m_targetPos - com).length();
    if (dist < 100.0f) {
        m_targetPos = {randFloat(50, 5950), randFloat(50, 5950)};
    }
    moveToward(self, m_targetPos);
}

void AIController::seekFood(Player& self, const World& world) {
    if (m_targetFoodIdx < 0 || m_targetFoodIdx >= world.foods().size()) {
        wander(self);
        return;
    }
    const auto& foods = world.foods();
    if (!foods[m_targetFoodIdx].alive) {
        m_targetFoodIdx = -1;
        wander(self);
        return;
    }
    moveToward(self, foods[m_targetFoodIdx].pos);
}

void AIController::chaseSmaller(Player& self, const World& world) {
    if (m_targetPlayerIdx < 0 || m_targetPlayerIdx >= world.players().size()) {
        wander(self);
        return;
    }
    const auto& players = world.players();
    if (players[m_targetPlayerIdx].totalMass() <= 0) {
        m_targetPlayerIdx = -1;
        wander(self);
        return;
    }
    moveToward(self, players[m_targetPlayerIdx].centerOfMass());
}

void AIController::fleeBigger(Player& self, const World& world) {
    if (m_threatPlayerIdx < 0 || m_threatPlayerIdx >= world.players().size()) {
        wander(self);
        return;
    }
    const auto& players = world.players();
    if (players[m_threatPlayerIdx].totalMass() <= 0) {
        m_threatPlayerIdx = -1;
        wander(self);
        return;
    }
    moveAwayFrom(self, players[m_threatPlayerIdx].centerOfMass());
}

void AIController::splitKill(Player& self, const World& world) {
    if (m_targetPlayerIdx < 0 || m_targetPlayerIdx >= world.players().size()) {
        wander(self);
        return;
    }
    const auto& players = world.players();
    if (players[m_targetPlayerIdx].totalMass() <= 0 || !self.canSplit()) {
        m_targetPlayerIdx = -1;
        wander(self);
        return;
    }

    Vec2 com = self.centerOfMass();
    Vec2 targetCom = players[m_targetPlayerIdx].centerOfMass();
    Vec2 dir = (targetCom - com).normalized();
    if (dir.lengthSq() < 0.001f) dir = {1.0f, 0.0f};

    float dist = (targetCom - com).length();
    float splitReach = 800.0f * 0.5f;

    if (dist < splitReach) {
        QVector<Cell> newCells;
        self.split(dir, newCells);
        for (auto& c : newCells) self.cells.append(c);
        m_state = AIState::ChaseSmaller;
    } else {
        moveToward(self, targetCom);
    }
}
