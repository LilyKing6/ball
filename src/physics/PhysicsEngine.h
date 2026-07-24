#ifndef PHYSICSENGINE_H
#define PHYSICSENGINE_H

#include "entity/Food.h"
#include "entity/Player.h"
#include "entity/BigBean.h"
#include "record/GameRecord.h"
#include <QVector>

class World;

struct KillResult {
    QString victimName;
    float victimMass;
    bool isSplitKill = false;
    bool isVirusKill = false;
};

class PhysicsEngine {
public:
    void updatePlayers(QVector<Player>& players, float dt, float worldW, float worldH);
    int checkFoodCollision(Player& player, QVector<Food>& foods);
    int checkBigBeanCollision(Player& player, QVector<BigBean>& beans);
    int checkPlayerCollision(Player& a, Player& b, QVector<KillResult>& outKills);
};

#endif // PHYSICSENGINE_H
