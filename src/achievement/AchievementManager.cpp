#include "AchievementManager.h"
#include "storage/DatabaseManager.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>

AchievementManager& AchievementManager::instance() {
    static AchievementManager s;
    return s;
}

AchievementManager::AchievementManager() {
    initDefinitions();
}

void AchievementManager::initDefinitions() {
    m_defs = {
        {"first_game",    "初出茅庐",   "完成你的第一局游戏",          AchievementCondition::TotalGames,       1,      RewardType::Title,   "newbie"},
        {"glutton",       "大胃王",     "单局吃掉100个食物",           AchievementCondition::FoodEatenSingle,  100,    RewardType::Skin,    "glutton"},
        {"slaughter",     "屠杀者",     "单局击杀10个玩家",            AchievementCondition::KillsSingle,      10,     RewardType::Title,   "killer"},
        {"behemoth",      "巨无霸",     "质量达到1000",               AchievementCondition::MaxMass,          1000,   RewardType::Skin,    "giant"},
        {"immortal",      "不死之身",   "存活超过10分钟",              AchievementCondition::SurvivalTime,     600,    RewardType::Glow,    "golden"},
        {"rank_up",       "段位晋升",   "首次达到黄金段位",             AchievementCondition::TierReach,        6,      RewardType::Skin,    "gold_beetle"},
        {"veteran",       "百战老兵",   "累计完成100局游戏",           AchievementCondition::TotalGames,       100,    RewardType::Title,   "veteran"},
        {"streak_king",   "连胜之王",   "达成5连胜",                  AchievementCondition::WinStreak,        5,      RewardType::Outline, "flame_border"},
        {"split_master",  "分裂大师",   "累计通过分裂击杀50次",         AchievementCondition::TotalSplitKills,  50,     RewardType::Title,   "shadow_clone"},
        {"virus_hunter",  "病毒猎人",   "累计利用病毒击杀20次",         AchievementCondition::TotalVirusKills,  20,     RewardType::Skin,    "poison_sting"},
        {"top1",          "冠军之路",   "累计10次排名第一",            AchievementCondition::TotalWins,        10,     RewardType::Title,   "king"},
        {"elo_2000",      "两千分",     "ELO达到2000分",             AchievementCondition::EloReach,         2000,   RewardType::Skin,    "diamond_glow"},
        {"mass_2000",     "超级巨无霸",  "质量达到2000",              AchievementCondition::MaxMass,          2000,   RewardType::Glow,    "nebula"},
        {"no_death",      "完美一局",   "零死亡且排名第一",             AchievementCondition::NoDeathRank1,     1,      RewardType::Title,   "undefeated"},
        {"marathon",      "马拉松",     "累计游戏时间达到10小时",        AchievementCondition::TotalPlayTime,    36000,  RewardType::Title,   "marathon"},
    };
}

void AchievementManager::loadUnlocked(const QString& playerId) {
    m_currentPlayerId = playerId;
    m_unlockedIds.clear();

    auto& db = DatabaseManager::instance().database();
    QSqlQuery q(db);
    q.prepare("SELECT item_id FROM unlock WHERE player_id = ? AND item_type = 'achievement'");
    q.addBindValue(playerId);
    if (q.exec()) {
        while (q.next()) {
            QString id = q.value("item_id").toString();
            m_unlockedIds.insert(id);
            for (auto& def : m_defs) {
                if (def.id == id) def.unlocked = true;
            }
        }
    }
}

bool AchievementManager::isUnlocked(const QString& playerId, const QString& achId) {
    return m_unlockedIds.contains(achId);
}

void AchievementManager::unlock(const QString& playerId, const AchievementDef& def) {
    auto& db = DatabaseManager::instance().database();
    QSqlQuery q(db);
    q.prepare("INSERT OR IGNORE INTO unlock (player_id, item_type, item_id, source) VALUES (?, 'achievement', ?, 'achievement')");
    q.addBindValue(playerId);
    q.addBindValue(def.id);
    if (q.exec() && q.numRowsAffected() > 0) {
        m_unlockedIds.insert(def.id);
        for (auto& d : m_defs) {
            if (d.id == def.id) d.unlocked = true;
        }
        m_pendingNotifications.append(def);
        qDebug() << "Achievement unlocked:" << def.name;
    }
}

void AchievementManager::checkSingleGame(const QString& playerId, const GameRecord& rec) {
    for (auto& def : m_defs) {
        if (def.unlocked) continue;
        bool met = false;
        switch (def.condType) {
        case AchievementCondition::FoodEatenSingle:
            met = rec.foodEaten >= static_cast<int>(def.condValue);
            break;
        case AchievementCondition::KillsSingle:
            met = rec.killCount >= static_cast<int>(def.condValue);
            break;
        case AchievementCondition::MaxMass:
            met = rec.maxMass >= def.condValue;
            break;
        case AchievementCondition::SurvivalTime:
            met = rec.duration >= def.condValue;
            break;
        case AchievementCondition::NoDeathRank1:
            met = rec.deathCause.isEmpty() && rec.rankInMatch == 1;
            break;
        default:
            break;
        }
        if (met) unlock(playerId, def);
    }
}

void AchievementManager::checkCumulative(const QString& playerId, const PlayerStats& stats) {
    for (auto& def : m_defs) {
        if (def.unlocked) continue;
        bool met = false;
        switch (def.condType) {
        case AchievementCondition::TotalGames:
            met = stats.totalGames >= static_cast<int>(def.condValue);
            break;
        case AchievementCondition::TotalWins:
            met = stats.bestRank == 1 ? (stats.totalGames - stats.totalDeaths) >= static_cast<int>(def.condValue) : false;
            break;
        case AchievementCondition::TotalPlayTime:
            met = stats.totalPlayTime >= def.condValue;
            break;
        case AchievementCondition::TotalSplitKills:
        case AchievementCondition::TotalVirusKills:
            // Not yet tracked in player_stats; will be 0
            break;
        default:
            break;
        }
        if (met) unlock(playerId, def);
    }
}

void AchievementManager::checkEloTier(const QString& playerId, int elo, const QString& tierName) {
    for (auto& def : m_defs) {
        if (def.unlocked) continue;
        bool met = false;
        switch (def.condType) {
        case AchievementCondition::EloReach:
            met = elo >= static_cast<int>(def.condValue);
            break;
        case AchievementCondition::TierReach: {
            // TierReach condValue = tier index (Gold_III = 6)
            int tierIdx = -1;
            QStringList tiers = {"Bronze_III","Bronze_II","Bronze_I",
                                 "Silver_III","Silver_II","Silver_I",
                                 "Gold_III","Gold_II","Gold_I",
                                 "Platinum_III","Platinum_II","Platinum_I",
                                 "Diamond_III","Diamond_II","Diamond_I",
                                 "Master_III","Master_II","Master_I",
                                 "King_III","King_II","King_I"};
            for (int i = 0; i < tiers.size(); i++) {
                if (tierName == tiers[i]) { tierIdx = i; break; }
            }
            met = tierIdx >= static_cast<int>(def.condValue);
            break;
        }
        default:
            break;
        }
        if (met) unlock(playerId, def);
    }
}

void AchievementManager::checkMassRealtime(const QString& playerId, float mass) {
    for (auto& def : m_defs) {
        if (def.unlocked) continue;
        if (def.condType == AchievementCondition::MaxMass && mass >= def.condValue) {
            unlock(playerId, def);
        }
    }
}

void AchievementManager::checkStreak(const QString& playerId, int currentStreak) {
    for (auto& def : m_defs) {
        if (def.unlocked) continue;
        if (def.condType == AchievementCondition::WinStreak && currentStreak >= static_cast<int>(def.condValue)) {
            unlock(playerId, def);
        }
    }
}

AchievementDef AchievementManager::popNotification() {
    if (m_pendingNotifications.isEmpty()) return {};
    return m_pendingNotifications.takeFirst();
}

int AchievementManager::unlockedCount() const {
    return m_unlockedIds.size();
}
