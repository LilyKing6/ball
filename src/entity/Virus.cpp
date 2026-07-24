#include "Virus.h"
#include "util/Config.h"

Virus::Virus() {
    auto& cfg = Config::instance();
    m_virusMass = cfg.virusMass;
    mass = m_virusMass;
}

Virus::Virus(const Vec2& pos) : Virus() {
    this->pos = pos;
}