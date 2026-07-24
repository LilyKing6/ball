#include "BigBean.h"
#include "util/Config.h"

float BigBean::radius() const {
    return Config::instance().radiusConstant * qSqrt(mass);
}
