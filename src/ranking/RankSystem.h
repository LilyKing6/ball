#ifndef RANKSYSTEM_H
#define RANKSYSTEM_H

#include <QString>
#include <QColor>

enum class RankTier {
    Bronze_III, Bronze_II, Bronze_I,
    Silver_III, Silver_II, Silver_I,
    Gold_II, Gold_I,
    Platinum_II, Platinum_I,
    Diamond_III, Diamond_II, Diamond_I,
    Master_III, Master_II, Master_I,
    King_III, King_II, King_I,
    SuperGod
};

struct TierInfo {
    QString name;
    int minElo;
    int maxElo;
    QColor color;
};

class RankSystem {
public:
    static RankTier tierFromElo(int elo);
    static TierInfo tierInfo(RankTier tier);
    static TierInfo tierInfoForElo(int elo);
    static int calculateEloChange(int winnerElo, int loserElo, float winnerMass, float loserMass);
    static float progressToNextTier(int elo);

    static constexpr int defaultElo = 1000;
};

#endif // RANKSYSTEM_H
