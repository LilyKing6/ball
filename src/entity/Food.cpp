#include "Food.h"
#include "util/Random.h"

Food::Food() {
    mass = 1.0f;
    color = QColor::fromHsv(randInt(0, 359), 180 + randInt(0, 75), 200 + randInt(0, 55));
}
