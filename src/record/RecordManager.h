#ifndef RECORDMANAGER_H
#define RECORDMANAGER_H

#include "GameRecord.h"
#include <QString>
#include <QVector>

class RecordManager {
public:
    static RecordManager& instance();

    bool ensureProfile(const QString& playerId, const QString& name);
    bool saveRecord(const QString& playerId, const GameRecord& record);
    bool updateStats(const QString& playerId, const GameRecord& record);
    QVector<GameRecord> loadRecords(const QString& playerId, int limit = 20);
    PlayerStats loadStats(const QString& playerId);

private:
    RecordManager() = default;
};

#endif // RECORDMANAGER_H
