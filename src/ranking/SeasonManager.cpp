#include "SeasonManager.h"
#include "RankSystem.h"
#include "storage/DatabaseManager.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>

SeasonManager& SeasonManager::instance() {
    static SeasonManager s;
    return s;
}

int SeasonManager::currentSeasonNumber() const {
    QDate today = QDate::currentDate();
    int daysSince = static_cast<int>(m_epoch.daysTo(today));
    return qMax(1, daysSince / m_seasonDays + 1);
}

QString SeasonManager::currentSeasonId() const {
    return QString("S%1").arg(currentSeasonNumber());
}

int SeasonManager::daysRemaining() const {
    QDate today = QDate::currentDate();
    int daysSince = static_cast<int>(m_epoch.daysTo(today));
    int daysIntoSeason = daysSince % m_seasonDays;
    return m_seasonDays - daysIntoSeason;
}

bool SeasonManager::checkSeasonRollover(const QString& playerId) {
    auto& db = DatabaseManager::instance().database();
    QString seasonId = currentSeasonId();

    QSqlQuery q(db);
    q.prepare("SELECT season_id FROM player_profile WHERE player_id = ?");
    q.addBindValue(playerId);
    if (q.exec() && q.next()) {
        QString storedSeason = q.value("season_id").toString();
        if (storedSeason != seasonId) {
            performSoftReset(playerId);
            QSqlQuery uq(db);
            uq.prepare("UPDATE player_profile SET season_id = ? WHERE player_id = ?");
            uq.addBindValue(seasonId);
            uq.addBindValue(playerId);
            uq.exec();
            return true;
        }
    }
    return false;
}

void SeasonManager::performSoftReset(const QString& playerId) {
    auto& db = DatabaseManager::instance().database();

    // Save old season record before reset
    QSqlQuery q(db);
    q.prepare("SELECT elo, rank_tier FROM player_profile WHERE player_id = ?");
    q.addBindValue(playerId);
    if (q.exec() && q.next()) {
        int oldElo = q.value("elo").toInt();
        QString oldTier = q.value("rank_tier").toString();

        QSqlQuery sq(db);
        sq.prepare("SELECT COUNT(*) as games FROM game_record WHERE player_id = ? AND season_id = ?");
        sq.addBindValue(playerId);
        QString oldSeason = QString("S%1").arg(currentSeasonNumber() - 1);
        sq.addBindValue(oldSeason);
        int games = 0;
        if (sq.exec() && sq.next()) games = sq.value("games").toInt();

        saveSeasonRecord(playerId, oldElo, oldTier, games, oldElo);
    }

    // Soft reset: newELO = 1500 + (oldELO - 1500) * 0.5
    int newElo = 1500 + qRound((q.value("elo").toInt() - 1500) * 0.5f);

    QSqlQuery uq(db);
    uq.prepare("UPDATE player_profile SET elo = ?, rank_tier = ? WHERE player_id = ?");
    uq.addBindValue(newElo);
    uq.addBindValue(RankSystem::tierInfoForElo(newElo).name);
    uq.addBindValue(playerId);
    uq.exec();
}

void SeasonManager::saveSeasonRecord(const QString& playerId, int peakElo, const QString& peakTier, int gamesPlayed, int finalElo) {
    auto& db = DatabaseManager::instance().database();
    QString seasonId = QString("S%1").arg(currentSeasonNumber() - 1);
    if (seasonId == "S0") return;

    QSqlQuery q(db);
    q.prepare(R"(
        INSERT OR REPLACE INTO season_record (season_id, player_id, start_date, end_date, peak_elo, peak_tier, games_played, final_elo)
        VALUES (?, ?, date('now','-30 days'), date('now'), ?, ?, ?, ?)
    )");
    q.addBindValue(seasonId);
    q.addBindValue(playerId);
    q.addBindValue(peakElo);
    q.addBindValue(peakTier);
    q.addBindValue(gamesPlayed);
    q.addBindValue(finalElo);
    q.exec();
}
