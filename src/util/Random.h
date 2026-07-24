#ifndef RANDOM_H
#define RANDOM_H

#include <QRandomGenerator>

inline float randFloat(float min, float max) {
    return min + QRandomGenerator::global()->generateDouble() * (max - min);
}

inline int randInt(int min, int max) {
    return QRandomGenerator::global()->bounded(min, max + 1);
}

#endif // RANDOM_H
