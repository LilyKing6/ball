#ifndef ACHIEVEMENTMANAGER_H
#define ACHIEVEMENTMANAGER_H

#include "AchievementDef.h"
#include "record/GameRecord.h"
#include <QVector>
#include <QString>
#include <QSet>

class AchievementManager {
public:
    static AchievementManager& instance();

    void loadUnlocked(const QString& playerId);

    void checkSingleGame(const QString& playerId, const GameRecord& rec);
    void checkCumulative(const QString& playerId, const PlayerStats& stats);
    void checkEloTier(const QString& playerId, int elo, const QString& tierName);
    void checkMassRealtime(const QString& playerId, float mass);
    void checkStreak(const QString& playerId, int currentStreak);

    const QVector<AchievementDef>& definitions() const { return m_defs; }

    bool hasPendingNotifications() const { return !m_pendingNotifications.isEmpty(); }
    AchievementDef popNotification();

    int unlockedCount() const;

private:
    AchievementManager();
    void initDefinitions();
    bool isUnlocked(const QString& playerId, const QString& achId);
    void unlock(const QString& playerId, const AchievementDef& def);

    QVector<AchievementDef> m_defs;
    QVector<AchievementDef> m_pendingNotifications;
    QSet<QString> m_unlockedIds;
    QString m_currentPlayerId;
};

#endif // ACHIEVEMENTMANAGER_H
