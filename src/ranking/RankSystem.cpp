#include "RankSystem.h"
#include <QtMath>
#include <algorithm>

static const TierInfo kTiers[] = {
    {"青铜 III",   0,   266, QColor("#CD7F32")},
    {"青铜 II",  267,   533, QColor("#CD7F32")},
    {"青铜 I",   534,   799, QColor("#CD7F32")},
    {"白银 III", 800,   933, QColor("#C0C0C0")},
    {"白银 II",  934,  1066, QColor("#C0C0C0")},
    {"白银 I",  1067,  1199, QColor("#C0C0C0")},
    {"黄金 II", 1200,  1399, QColor("#FFD700")},
    {"黄金 I",  1400,  1599, QColor("#FFD700")},
    {"铂金 II", 1600,  1799, QColor("#E5E4E2")},
    {"铂金 I",  1800,  1999, QColor("#E5E4E2")},
    {"钻石 III",2000,  2166, QColor("#B9F2FF")},
    {"钻石 II", 2167,  2333, QColor("#B9F2FF")},
    {"钻石 I",  2334,  2499, QColor("#B9F2FF")},
    {"大师 III",2500,  2666, QColor("#FF4500")},
    {"大师 II", 2667,  2833, QColor("#FF4500")},
    {"大师 I",  2834,  2999, QColor("#FF4500")},
    {"王者 III",3000,  3166, QColor("#FF0000")},
    {"王者 II", 3167,  3333, QColor("#FF0000")},
    {"王者 I",  3334,  3499, QColor("#FF0000")},
    {"超神",    3500,  9999, QColor("#FF00FF")},
};

static constexpr int kTierCount = sizeof(kTiers) / sizeof(kTiers[0]);

RankTier RankSystem::tierFromElo(int elo) {
    for (int i = 0; i < kTierCount; i++) {
        if (elo >= kTiers[i].minElo && elo <= kTiers[i].maxElo)
            return static_cast<RankTier>(i);
    }
    return RankTier::King_I;
}

TierInfo RankSystem::tierInfo(RankTier tier) {
    int idx = static_cast<int>(tier);
    if (idx < 0 || idx >= kTierCount) return kTiers[0];
    return kTiers[idx];
}

TierInfo RankSystem::tierInfoForElo(int elo) {
    return tierInfo(tierFromElo(elo));
}

int RankSystem::calculateEloChange(int winnerElo, int loserElo, float winnerMass, float loserMass) {
    float expected = 1.0f / (1.0f + qPow(10.0f, (loserElo - winnerElo) / 400.0f));
    float score = 1.0f - expected;

    int K;
    if (winnerElo < 1200)       K = 32;
    else if (winnerElo < 2000)  K = 24;
    else                        K = 16;

    float massRatio = loserMass / qMax(winnerMass, 1.0f);
    float massFactor = std::clamp(massRatio, 0.5f, 2.0f);

    return qRound(score * K * massFactor);
}

float RankSystem::progressToNextTier(int elo) {
    RankTier t = tierFromElo(elo);
    TierInfo info = tierInfo(t);
    if (t == RankTier::King_I) return 1.0f;
    TierInfo next = tierInfo(static_cast<RankTier>(static_cast<int>(t) + 1));
    float range = static_cast<float>(next.minElo - info.minElo);
    float progress = static_cast<float>(elo - info.minElo);
    return progress / range;
}
