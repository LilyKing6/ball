#ifndef PLAYER_H
#define PLAYER_H

#include "Entity.h"
#include "Spore.h"
#include <QString>
#include <QColor>
#include <QVector>

class Cell : public Entity {
public:
    Cell();
    float radius() const;
    QColor color;
    float splitCooldown = 0.0f;
    bool isMerging = false;
    float mergeTimer = 0.0f;
    float invincibleTimer = 0.0f;  // 出生无敌倒计时（>0 时免疫被吃）
};

class Player {
public:
    Player();

    QString name;
    QVector<Cell> cells;
    Vec2 targetDir;
    Vec2 mouseWorldPos;     // 兼容字段（原始鼠标位置，AI 也用此）
    Vec2 virtualCursor;     // 虚拟游标位置：移动+吐球目标点
    int id = 0;
    int kills = 0;
    int team = 0; // 0=none, 1=TeamA, 2=TeamB
    bool isAI = false;

    // 模式倍率（由 World::init 从 GameModeConfig 设置）
    float speedMul = 1.0f;
    float splitVelocityMul = 1.0f;
    float splitCooldownMul = 1.0f;
    float mergeCooldownMul = 1.0f;

    // 防护盾系统（仅在有安全区的模式中生效）
    int shieldCount = 0;
    float shieldDecayTimer = 0.0f;  // 圈外计时器，达 5s 触发扣盾

    float totalMass() const;
    Vec2 centerOfMass() const;
    void update(float dt);

    bool canSplit() const;
    void split(Vec2 direction, QVector<Cell>& newCells);
    void eject(Vec2 direction, QVector<Spore>& ejected);
    void ejectToward(Vec2 worldTarget, QVector<Spore>& ejected); // 聚球：朝目标位置吐

private:
    float m_ejectCooldown = 0.0f;
};

#endif // PLAYER_H
