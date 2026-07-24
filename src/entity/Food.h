#ifndef FOOD_H
#define FOOD_H

#include "Entity.h"
#include <QColor>

class Food : public Entity {
public:
    Food();

    QColor color;
};

#endif // FOOD_H
