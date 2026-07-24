#ifndef ACHIEVEMENTDEF_H
#define ACHIEVEMENTDEF_H

#include <QString>

enum class AchievementCondition {
    TotalGames,
    FoodEatenSingle,
    KillsSingle,
    MaxMass,
    SurvivalTime,
    NoDeathRank1,
    EloReach,
    TierReach,
    WinStreak,
    TotalSplitKills,
    TotalVirusKills,
    TotalWins,
    TotalPlayTime,
};

enum class RewardType {
    Title,
    Skin,
    Glow,
    Outline,
};

struct AchievementDef {
    QString id;
    QString name;
    QString description;
    AchievementCondition condType;
    float condValue = 0;
    RewardType rewardType = RewardType::Title;
    QString rewardId;
    bool unlocked = false;
};

#endif // ACHIEVEMENTDEF_H
