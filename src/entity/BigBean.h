#ifndef BIGBEAN_H
#define BIGBEAN_H

#include "util/Math.h"
#include <QColor>

struct BigBean {
    Vec2 pos;
    float mass = 100.0f;
    bool alive = true;
    QColor color;
    float pulsePhase = 0.0f; // for pulsating animation

    float radius() const;
};

#endif // BIGBEAN_H
