#include "PhysicsEngine.h"
#include "util/Config.h"
#include <algorithm>

void PhysicsEngine::updatePlayers(QVector<Player>& players, float dt, float worldW, float worldH) {
    auto& cfg = Config::instance();
    for (auto& p : players) {
        p.update(dt);
        for (auto& c : p.cells) {
            c.pos.x = clamp(c.pos.x, c.radius(), worldW - c.radius());
            c.pos.y = clamp(c.pos.y, c.radius(), worldH - c.radius());
        }
    }
}

int PhysicsEngine::checkFoodCollision(Player& player, QVector<Food>& foods) {
    int eaten = 0;
    auto& cfg = Config::instance();
    for (auto& c : player.cells) {
        float r = c.radius();
        for (auto& f : foods) {
            if (!f.alive) continue;
            float d = (f.pos - c.pos).length();
            if (d < r) {
                c.mass += f.mass;
                if (c.mass > cfg.maxMassPerCell) c.mass = cfg.maxMassPerCell;
                f.alive = false;
                eaten++;
            }
        }
    }
    return eaten;
}

int PhysicsEngine::checkPlayerCollision(Player& a, Player& b, QVector<KillResult>& outKills) {
    int kills = 0;
    auto& cfg = Config::instance();

    // Friendly-fire prevention: same team can't eat each other
    if (a.team != 0 && a.team == b.team) return 0;

    // a eats b
    for (auto& ca : a.cells) {
        if (!ca.alive) continue;
        for (auto& cb : b.cells) {
            if (!cb.alive) continue;
            // 出生无敌：被吃方在无敌时免疫
            if (cb.invincibleTimer > 0) continue;
            if (ca.mass > cb.mass * cfg.massRatioForEat) {
                float d = (ca.pos - cb.pos).length();
                if (d < ca.radius() - cb.radius() * cfg.overlapRatioForEat) {
                    float victimMass = cb.mass;
                    QString victimName = b.name;
                    bool isSplitKill = a.splitTimer > 0.0f;
                    bool isVirusKill = b.virusHitTimer > 0.0f;
                    ca.mass += cb.mass;
                    if (ca.mass > cfg.maxMassPerCell) ca.mass = cfg.maxMassPerCell;
                    cb.alive = false;
                    kills++;
                    outKills.append({victimName, victimMass, isSplitKill, isVirusKill});
                }
            }
        }
    }

    // b eats a
    for (auto& cb : b.cells) {
        if (!cb.alive) continue;
        for (auto& ca : a.cells) {
            if (!ca.alive) continue;
            if (ca.invincibleTimer > 0) continue;
            if (cb.mass > ca.mass * cfg.massRatioForEat) {
                float d = (cb.pos - ca.pos).length();
                if (d < cb.radius() - ca.radius() * cfg.overlapRatioForEat) {
                    float victimMass = ca.mass;
                    QString victimName = a.name;
                    bool isSplitKill = b.splitTimer > 0.0f;
                    bool isVirusKill = a.virusHitTimer > 0.0f;
                    cb.mass += ca.mass;
                    if (cb.mass > cfg.maxMassPerCell) cb.mass = cfg.maxMassPerCell;
                    ca.alive = false;
                    kills++;
                    outKills.append({victimName, victimMass, isSplitKill, isVirusKill});
                }
            }
        }
    }

    // 防护盾继承：被全歼方的盾转移给吃掉方
    bool aAllDead = std::all_of(a.cells.begin(), a.cells.end(),
        [](const Cell& c) { return !c.alive; });
    bool bAllDead = std::all_of(b.cells.begin(), b.cells.end(),
        [](const Cell& c) { return !c.alive; });
    if (aAllDead && !bAllDead && a.shieldCount > 0) {
        b.shieldCount += a.shieldCount;
        a.shieldCount = 0;
    } else if (bAllDead && !aAllDead && b.shieldCount > 0) {
        a.shieldCount += b.shieldCount;
        b.shieldCount = 0;
    }

    return kills;
}

int PhysicsEngine::checkBigBeanCollision(Player& player, QVector<BigBean>& beans) {
    int eaten = 0;
    auto& cfg = Config::instance();
    for (auto& c : player.cells) {
        float r = c.radius();
        for (auto& b : beans) {
            if (!b.alive) continue;
            float d = (b.pos - c.pos).length();
            if (d < r + b.radius() * 0.5f) {
                c.mass += b.mass;
                if (c.mass > cfg.maxMassPerCell) c.mass = cfg.maxMassPerCell;
                b.alive = false;
                eaten++;
            }
        }
    }
    return eaten;
}
