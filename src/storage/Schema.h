#ifndef SCHEMA_H
#define SCHEMA_H

#include <QString>

namespace Schema {

inline QString createTables() {
    return R"(
        CREATE TABLE IF NOT EXISTS player_profile (
            player_id   TEXT PRIMARY KEY,
            name        TEXT NOT NULL,
            elo         INTEGER DEFAULT 1000,
            rank_tier   TEXT DEFAULT 'Bronze_III',
            season_id   TEXT,
            created_at  DATETIME DEFAULT CURRENT_TIMESTAMP,
            updated_at  DATETIME DEFAULT CURRENT_TIMESTAMP
        );

        CREATE TABLE IF NOT EXISTS game_record (
            record_id     TEXT PRIMARY KEY,
            player_id     TEXT,
            timestamp     DATETIME DEFAULT CURRENT_TIMESTAMP,
            duration      REAL,
            final_mass    REAL,
            max_mass      REAL,
            kill_count    INTEGER DEFAULT 0,
            death_cause   TEXT,
            killed_by     TEXT,
            food_eaten    INTEGER DEFAULT 0,
            split_count   INTEGER DEFAULT 0,
            eject_count   INTEGER DEFAULT 0,
            elo_change    INTEGER DEFAULT 0,
            rank_in_match INTEGER,
            total_players INTEGER,
            mode          TEXT DEFAULT 'single',
            season_id     TEXT,
            FOREIGN KEY(player_id) REFERENCES player_profile(player_id)
        );

        CREATE TABLE IF NOT EXISTS kill_detail (
            id            INTEGER PRIMARY KEY AUTOINCREMENT,
            record_id     TEXT,
            victim_name   TEXT,
            kill_time     REAL,
            victim_mass   REAL,
            FOREIGN KEY(record_id) REFERENCES game_record(record_id)
        );

        CREATE TABLE IF NOT EXISTS season_record (
            season_id     TEXT,
            player_id     TEXT,
            start_date    DATETIME,
            end_date      DATETIME,
            peak_elo      INTEGER,
            peak_tier     TEXT,
            games_played  INTEGER DEFAULT 0,
            final_elo     INTEGER,
            PRIMARY KEY(season_id, player_id)
        );

        CREATE TABLE IF NOT EXISTS player_stats (
            player_id         TEXT PRIMARY KEY,
            total_games       INTEGER DEFAULT 0,
            total_kills       INTEGER DEFAULT 0,
            total_deaths      INTEGER DEFAULT 0,
            best_mass         REAL DEFAULT 0,
            best_rank         INTEGER DEFAULT 999,
            longest_survival  REAL DEFAULT 0,
            total_food_eaten  INTEGER DEFAULT 0,
            total_play_time   REAL DEFAULT 0,
            best_streak       INTEGER DEFAULT 0,
            current_streak    INTEGER DEFAULT 0,
            total_split_kills INTEGER DEFAULT 0,
            total_virus_kills INTEGER DEFAULT 0,
            FOREIGN KEY(player_id) REFERENCES player_profile(player_id)
        );

        CREATE TABLE IF NOT EXISTS unlock (
            id          INTEGER PRIMARY KEY AUTOINCREMENT,
            player_id   TEXT,
            item_type   TEXT,
            item_id     TEXT,
            unlocked_at DATETIME DEFAULT CURRENT_TIMESTAMP,
            source      TEXT,
            UNIQUE(player_id, item_type, item_id)
        );
    )";
}

} // namespace Schema

#endif // SCHEMA_H
