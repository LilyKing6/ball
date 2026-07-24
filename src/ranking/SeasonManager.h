#ifndef SEASONMANAGER_H
#define SEASONMANAGER_H

#include <QString>
#include <QDate>

class SeasonManager {
public:
    static SeasonManager& instance();

    QString currentSeasonId() const;
    int daysRemaining() const;
    bool checkSeasonRollover(const QString& playerId);
    void performSoftReset(const QString& playerId);
    void saveSeasonRecord(const QString& playerId, int peakElo, const QString& peakTier, int gamesPlayed, int finalElo);

private:
    SeasonManager() = default;
    QDate m_epoch = QDate(2026, 1, 1);
    int m_seasonDays = 30;

    int currentSeasonNumber() const;
};

#endif // SEASONMANAGER_H
