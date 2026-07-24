#ifndef GAMERECORD_H
#define GAMERECORD_H

#include <QString>
#include <QDateTime>
#include <QVector>

struct KillEvent {
    QString victimName;
    float killTime;
    float victimMass;
};

struct GameRecord {
    QString recordId;
    QDateTime timestamp;
    float duration = 0;
    float finalMass = 0;
    float maxMass = 0;
    int killCount = 0;
    QString deathCause;
    QString killedBy;
    int foodEaten = 0;
    int splitCount = 0;
    int ejectCount = 0;
    int eloChange = 0;
    int rankInMatch = 0;
    int totalPlayers = 0;
    QString mode = "single";
    QString seasonId;
    QVector<KillEvent> killTimeline;
};

struct PlayerStats {
    int totalGames = 0;
    int totalKills = 0;
    int totalDeaths = 0;
    float bestMass = 0;
    int bestRank = 999;
    float longestSurvival = 0;
    int totalFoodEaten = 0;
    float totalPlayTime = 0;
    int bestStreak = 0;
    int currentStreak = 0;
};

#endif // GAMERECORD_H
