#include "Virus.h"
#include "util/Config.h"
#include <QColor>

Virus::Virus() {
    auto& cfg = Config::instance();
    m_virusMass = cfg.virusMass;
    mass = m_virusMass;
}

Virus::Virus(const Vec2& pos) : Virus() {
    this->pos = pos;
}

Virus::Virus(const Vec2& pos, VirusType type) : Virus() {
    this->pos = pos;
    m_type = type;
    auto& cfg = Config::instance();
    if (type == VirusType::Big) {
        m_virusMass = cfg.virusMass * 2.0f;
        mass = m_virusMass;
    } else if (type == VirusType::Exploder || type == VirusType::Poison) {
        m_virusMass = cfg.virusMass * 0.8f;
        mass = m_virusMass;
    }
}

QColor Virus::color() const {
    switch (m_type) {
    case VirusType::Normal:   return QColor(255, 0, 0);       // 红
    case VirusType::Exploder: return QColor(255, 127, 0);     // 橙
    case VirusType::Big:      return QColor(100, 200, 255);    // 蓝
    case VirusType::Poison:   return QColor(128, 0, 128);     // 紫
    }
    return QColor(255, 0, 0);
}
