#include "RecordManager.h"
#include "storage/DatabaseManager.h"
#include "achievement/AchievementManager.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QUuid>
#include <QDebug>

RecordManager& RecordManager::instance() {
    static RecordManager s;
    return s;
}

bool RecordManager::ensureProfile(const QString& playerId, const QString& name) {
    auto& db = DatabaseManager::instance().database();
    QSqlQuery q(db);
    q.prepare("INSERT OR IGNORE INTO player_profile (player_id, name) VALUES (?, ?)");
    q.addBindValue(playerId);
    q.addBindValue(name);
    if (!q.exec()) {
        qWarning() << "ensureProfile:" << q.lastError().text();
        return false;
    }
    return true;
}

bool RecordManager::saveRecord(const QString& playerId, const GameRecord& rec) {
    auto& db = DatabaseManager::instance().database();
    QSqlQuery q(db);

    QString recordId = QUuid::createUuid().toString(QUuid::WithoutBraces);
    q.prepare(R"(
        INSERT INTO game_record (record_id, player_id, duration, final_mass, max_mass,
            kill_count, split_kill_count, virus_kill_count, death_cause, killed_by,
            food_eaten, split_count, eject_count, elo_change, rank_in_match,
            total_players, mode, season_id)
        VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
    )");
    q.addBindValue(recordId);
    q.addBindValue(playerId);
    q.addBindValue(rec.duration);
    q.addBindValue(rec.finalMass);
    q.addBindValue(rec.maxMass);
    q.addBindValue(rec.killCount);
    q.addBindValue(rec.splitKillCount);
    q.addBindValue(rec.virusKillCount);
    q.addBindValue(rec.deathCause);
    q.addBindValue(rec.killedBy);
    q.addBindValue(rec.foodEaten);
    q.addBindValue(rec.splitCount);
    q.addBindValue(rec.ejectCount);
    q.addBindValue(rec.eloChange);
    q.addBindValue(rec.rankInMatch);
    q.addBindValue(rec.totalPlayers);
    q.addBindValue(rec.mode);
    q.addBindValue(rec.seasonId);

    if (!q.exec()) {
        qWarning() << "saveRecord:" << q.lastError().text();
        return false;
    }

    for (auto& k : rec.killTimeline) {
        QSqlQuery kq(db);
        kq.prepare("INSERT INTO kill_detail (record_id, victim_name, kill_time, victim_mass, is_split_kill, is_virus_kill) VALUES (?, ?, ?, ?, ?, ?)");
        kq.addBindValue(recordId);
        kq.addBindValue(k.victimName);
        kq.addBindValue(k.killTime);
        kq.addBindValue(k.victimMass);
        kq.addBindValue(k.isSplitKill ? 1 : 0);
        kq.addBindValue(k.isVirusKill ? 1 : 0);
        kq.exec();
    }

    return true;
}

bool RecordManager::updateStats(const QString& playerId, const GameRecord& rec) {
    auto& db = DatabaseManager::instance().database();

    // Read current streak values
    int currentStreak = 0, bestStreak = 0;
    QSqlQuery sq(db);
    sq.prepare("SELECT current_streak, best_streak FROM player_stats WHERE player_id = ?");
    sq.addBindValue(playerId);
    if (sq.exec() && sq.next()) {
        currentStreak = sq.value("current_streak").toInt();
        bestStreak = sq.value("best_streak").toInt();
    }

    int deaths = rec.deathCause.isEmpty() ? 0 : 1;
    bool isWin = (deaths == 0 && rec.rankInMatch == 1);
    int newStreak = isWin ? currentStreak + 1 : 0;
    int newBest = qMax(bestStreak, newStreak);

    QSqlQuery q(db);
    q.prepare(R"(
        INSERT INTO player_stats (player_id, total_games, total_kills, total_deaths,
            best_mass, best_rank, longest_survival, total_food_eaten, total_play_time,
            best_streak, current_streak, total_split_kills, total_virus_kills)
        VALUES (?, 1, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
        ON CONFLICT(player_id) DO UPDATE SET
            total_games = total_games + 1,
            total_kills = total_kills + ?,
            total_deaths = total_deaths + ?,
            best_mass = MAX(best_mass, ?),
            best_rank = MIN(best_rank, ?),
            longest_survival = MAX(longest_survival, ?),
            total_food_eaten = total_food_eaten + ?,
            total_play_time = total_play_time + ?,
            best_streak = MAX(best_streak, ?),
            current_streak = ?,
            total_split_kills = total_split_kills + ?,
            total_virus_kills = total_virus_kills + ?
    )");

    q.addBindValue(playerId);
    q.addBindValue(rec.killCount);
    q.addBindValue(deaths);
    q.addBindValue(rec.maxMass);
    q.addBindValue(rec.rankInMatch);
    q.addBindValue(rec.duration);
    q.addBindValue(rec.foodEaten);
    q.addBindValue(rec.duration);
    q.addBindValue(newBest);
    q.addBindValue(newStreak);
    q.addBindValue(rec.splitKillCount);
    q.addBindValue(rec.virusKillCount);
    // UPDATE values
    q.addBindValue(rec.killCount);
    q.addBindValue(deaths);
    q.addBindValue(rec.maxMass);
    q.addBindValue(rec.rankInMatch);
    q.addBindValue(rec.duration);
    q.addBindValue(rec.foodEaten);
    q.addBindValue(rec.duration);
    q.addBindValue(newBest);
    q.addBindValue(newStreak);
    q.addBindValue(rec.splitKillCount);
    q.addBindValue(rec.virusKillCount);

    if (!q.exec()) {
        qWarning() << "updateStats:" << q.lastError().text();
        return false;
    }

    // Check cumulative + streak achievements
    PlayerStats updated = loadStats(playerId);
    auto& ach = AchievementManager::instance();
    ach.checkCumulative(playerId, updated);
    ach.checkStreak(playerId, updated.currentStreak);

    return true;
}

QVector<GameRecord> RecordManager::loadRecords(const QString& playerId, int limit) {
    QVector<GameRecord> records;
    auto& db = DatabaseManager::instance().database();
    QSqlQuery q(db);
    q.prepare("SELECT * FROM game_record WHERE player_id = ? ORDER BY timestamp DESC LIMIT ?");
    q.addBindValue(playerId);
    q.addBindValue(limit);

    if (q.exec()) {
        while (q.next()) {
            GameRecord r;
            r.recordId = q.value("record_id").toString();
            r.timestamp = q.value("timestamp").toDateTime();
            r.duration = q.value("duration").toFloat();
            r.finalMass = q.value("final_mass").toFloat();
            r.maxMass = q.value("max_mass").toFloat();
            r.killCount = q.value("kill_count").toInt();
            r.splitKillCount = q.value("split_kill_count").toInt();
            r.virusKillCount = q.value("virus_kill_count").toInt();
            r.deathCause = q.value("death_cause").toString();
            r.killedBy = q.value("killed_by").toString();
            r.foodEaten = q.value("food_eaten").toInt();
            r.splitCount = q.value("split_count").toInt();
            r.ejectCount = q.value("eject_count").toInt();
            r.eloChange = q.value("elo_change").toInt();
            r.rankInMatch = q.value("rank_in_match").toInt();
            r.totalPlayers = q.value("total_players").toInt();
            r.mode = q.value("mode").toString();
            r.seasonId = q.value("season_id").toString();
            records.append(r);
        }
    }
    return records;
}

PlayerStats RecordManager::loadStats(const QString& playerId) {
    PlayerStats s;
    auto& db = DatabaseManager::instance().database();
    QSqlQuery q(db);
    q.prepare("SELECT * FROM player_stats WHERE player_id = ?");
    q.addBindValue(playerId);
    if (q.exec() && q.next()) {
        s.totalGames = q.value("total_games").toInt();
        s.totalKills = q.value("total_kills").toInt();
        s.totalDeaths = q.value("total_deaths").toInt();
        s.bestMass = q.value("best_mass").toFloat();
        s.bestRank = q.value("best_rank").toInt();
        s.longestSurvival = q.value("longest_survival").toFloat();
        s.totalFoodEaten = q.value("total_food_eaten").toInt();
        s.totalPlayTime = q.value("total_play_time").toFloat();
        s.bestStreak = q.value("best_streak").toInt();
        s.currentStreak = q.value("current_streak").toInt();
        s.totalSplitKills = q.value("total_split_kills").toInt();
        s.totalVirusKills = q.value("total_virus_kills").toInt();
    }
    return s;
}
