#ifndef WORLD_H
#define WORLD_H

#include <QtCore>
#include "util/Math.h"
#include "entity/Player.h"
#include "entity/Food.h"
#include "entity/Spore.h"
#include "entity/Virus.h"
#include "entity/BigBean.h"
#include "ai/AIController.h"
#include "physics/PhysicsEngine.h"
#include "engine/GameMode.h"

struct FrameKill {
    QString victimName;
    float victimMass;
};

class Spawner;

class World {
public:
    World();

    float width() const { return m_width; }
    float height() const { return m_height; }
    void setSize(float w, float h) { m_width = w; m_height = h; }

    Vec2 clampPosition(const Vec2& pos) const;
    bool isInBounds(const Vec2& pos) const;

    QVector<Player>& players() { return m_players; }
    const QVector<Player>& players() const { return m_players; }
    QVector<Food>& foods() { return m_foods; }
    const QVector<Food>& foods() const { return m_foods; }
    QVector<Spore>& spores() { return m_spores; }
    const QVector<Spore>& spores() const { return m_spores; }
    QVector<Virus>& viruses() { return m_viruses; }
    const QVector<Virus>& viruses() const { return m_viruses; }
    QVector<BigBean>& bigBeans() { return m_bigBeans; }
    const QVector<BigBean>& bigBeans() const { return m_bigBeans; }

    Player* localPlayer() { return m_localPlayerIdx >= 0 && m_localPlayerIdx < m_players.size() ? &m_players[m_localPlayerIdx] : nullptr; }
    const Player* localPlayer() const { return m_localPlayerIdx >= 0 && m_localPlayerIdx < m_players.size() ? &m_players[m_localPlayerIdx] : nullptr; }
    void setLocalPlayer(int idx) { m_localPlayerIdx = idx; }

    void update(float dt);
    void init(GameMode mode = GameMode::FreeMode);

    // 网络模式：用服务端 snapshot 覆盖 World 状态
    void applySnapshot(const struct WorldSnapshot& snap, int myPlayerId);

    int frameFoodEaten() const { return m_frameFoodEaten; }
    const QVector<FrameKill>& frameKills() const { return m_frameKills; }
    bool localPlayerDied() const { return m_localPlayerDied; }
    bool localPlayerWon() const { return m_localPlayerWon; }
    bool teamAWon() const { return m_teamAWon; }
    bool teamBWon() const { return m_teamBWon; }
    float safeZoneRadius() const { return m_safeZoneRadius; }
    Vec2 safeZoneCenter() const { return m_safeZoneCenter; }
    float shrinkTimer() const { return m_shrinkTimer; }
    float timeToNextShrink() const;     // 距下次缩圈秒数（-1 表示已结束）
    int currentShrinkPhase() const { return m_shrinkPhase; }  // 已完成的缩圈阶段
    bool inShrinkWarning() const;       // 是否在缩圈警告期内（<5s）
    float teamAMass() const;
    float teamBMass() const;
    float gameTime() const { return m_gameTime; }
    float localRespawnTimer() const;
    GameMode currentMode() const { return m_currentMode; }
    const GameModeConfig& modeConfig() const { return m_modeCfg; }
    void resetFrameCounters() { m_frameFoodEaten = 0; m_frameKills.clear(); m_localPlayerDied = false; m_localPlayerWon = false; m_teamAWon = false; m_teamBWon = false; }

private:
    float m_width = 6000.0f;
    float m_height = 6000.0f;
    QVector<Player> m_players;
    QVector<Food> m_foods;
    QVector<Spore> m_spores;
    QVector<Virus> m_viruses;
    QVector<BigBean> m_bigBeans;
    QVector<AIController> m_aiControllers;
    PhysicsEngine m_physics;
    int m_localPlayerIdx = -1;

    int m_frameFoodEaten = 0;
    QVector<FrameKill> m_frameKills;
    bool m_localPlayerDied = false;

    struct RespawnEntry {
        int playerId;
        QString name;
        AIDifficulty difficulty;
        float timer;
        bool isAI;
        int team = 0;
    };
    QVector<RespawnEntry> m_respawnQueue;
    int m_nextPlayerId = 1000;

    // Win condition flags
    bool m_localPlayerWon = false;
    bool m_teamAWon = false;
    bool m_teamBWon = false;

    // BattleRoyale shrinking zone
    float m_safeZoneRadius = 0;
    Vec2 m_safeZoneCenter;
    float m_shrinkTimer = 0;
    int m_shrinkPhase = 0;            // 已完成的缩圈次数
    float m_minSafeZoneRadius = 200.0f;
    float m_gameTime = 0;
    GameMode m_currentMode = GameMode::FreeMode;
    GameModeConfig m_modeCfg;
};

#endif // WORLD_H
