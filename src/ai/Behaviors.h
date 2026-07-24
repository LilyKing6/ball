#ifndef BEHAVIORS_H
#define BEHAVIORS_H

enum class AIState {
    Wander,
    SeekFood,
    ChaseSmaller,
    FleeBigger,
    SplitKill
};

enum class AIDifficulty {
    Easy,
    Normal,
    Hard
};

struct AIConfig {
    float perceptionRadius;
    float reactionDelay;
    bool canSplit;
    float splitSkill; // 0-1

    static AIConfig forDifficulty(AIDifficulty diff) {
        switch (diff) {
        case AIDifficulty::Easy:   return {600, 0.5f, false, 0.0f};
        case AIDifficulty::Normal: return {800, 0.2f, true,  0.3f};
        case AIDifficulty::Hard:   return {1200, 0.05f, true, 0.8f};
        }
        return {800, 0.2f, false, 0.0f};
    }
};

#endif // BEHAVIORS_H
