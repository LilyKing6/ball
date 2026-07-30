#ifndef VIRUS_H
#define VIRUS_H

#include "Entity.h"
#include <QColor>

enum class VirusType {
    Normal,    // 默认刺球：被吃者分裂成碎片
    Exploder,  // 爆裂刺球：直接炸开被吃者 cell，不分裂
    Big,       // 大刺球：体积/质量更大，分裂成更多碎片
    Poison,    // 毒刺球：被吃者持续损失质量
};

class Virus : public Entity {
public:
    Virus();
    Virus(const Vec2& pos);
    Virus(const Vec2& pos, VirusType type);

    float getMass() const { return m_virusMass; }
    void setTriggered() { m_triggered = true; }
    bool isTriggered() const { return m_triggered; }

    float respawnTimer = 10.0f;

    VirusType type() const { return m_type; }
    QColor color() const;

private:
    float m_virusMass = 100.0f;
    bool m_triggered = false;
    VirusType m_type = VirusType::Normal;
};

#endif // VIRUS_H
