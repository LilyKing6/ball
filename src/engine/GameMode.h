#ifndef GAMEMODE_H
#define GAMEMODE_H

#include <QString>

enum class GameMode {
    FreeMode,       // 自由模式
    SpeedFree,      // 极速自由
    TeamMode,       // 团战模式
    BattleRoyale    // 大逃杀
};

struct GameModeConfig {
    QString name;
    QString description;
    float worldWidth;
    float worldHeight;
    int aiCount;
    int foodCount;
    int virusCount;
    bool hasTeams;
    bool hasShrinkingZone;
    int timeLimitSeconds;  // 0 = no limit

    // 模式倍率（极速模式专用，其他模式保持 1.0）
    float speedMultiplier = 1.0f;       // 移动速度倍率
    float splitVelocityMul = 1.0f;      // 分裂初速度倍率
    float splitCooldownMul = 1.0f;      // 分裂冷却倍率
    float mergeCooldownMul = 1.0f;      // 融合冷却倍率

    // 大逃杀缩圈参数
    int brShrinkPhases = 5;             // 总缩圈次数
    float brShrinkInterval = 30.0f;     // 每次缩圈间隔（秒）
    float brShrinkFactor = 0.85f;       // 每次缩圈保留比例
    float brZoneDamagePerSec = 50.0f;   // 圈外每秒扣质量
    float brShrinkWarnTime = 5.0f;      // 缩圈前警告时间
};

inline GameModeConfig getModeConfig(GameMode mode) {
    switch (mode) {
    case GameMode::FreeMode:
        return {"自由模式", "经典大乱斗，30个AI对手，自由发育吞噬对手", 3000, 3000, 30, 2000, 20, false, false, 600};
    case GameMode::SpeedFree:
        return {"极速自由", "快节奏模式，20个AI对手，速度×1.3，分裂与融合冷却减半",
                2000, 2000, 20, 1500, 15, false, false, 300,
                1.3f, 1.3f, 0.5f, 0.5f};  // 速度/分裂速度/分裂冷却/融合冷却
    case GameMode::TeamMode:
        return {"团战模式", "团队对抗，AI分为两队，时限内质量高者获胜", 4000, 4000, 20, 2500, 25, true, false, 600};
    case GameMode::BattleRoyale:
        return {"大逃杀", "生存模式，安全区阶梯式缩小，活到最后", 3500, 3500, 30, 2000, 20, false, true, 480};
    }
    return {"自由模式", "", 3000, 3000, 30, 2000, 20, false, false, 600};
}

#endif // GAMEMODE_H
