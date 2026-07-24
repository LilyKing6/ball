#ifndef SPAWNER_H
#define SPAWNER_H

#include "Food.h"
#include "Player.h"
#include "util/Config.h"
#include "util/Random.h"
#include <QVector>

class Spawner {
public:
    static void spawnFood(QVector<Food>& foods, int targetCount, float worldW, float worldH) {
        while (foods.size() < targetCount) {
            Food f;
            f.pos = {randFloat(10, worldW - 10), randFloat(10, worldH - 10)};
            foods.append(f);
        }
    }

    static void respawnFood(QVector<Food>& foods, int targetCount, float worldW, float worldH) {
        foods.erase(std::remove_if(foods.begin(), foods.end(), [](const Food& f) {
            return !f.alive;
        }), foods.end());
        spawnFood(foods, targetCount, worldW, worldH);
    }
};

#endif // SPAWNER_H
