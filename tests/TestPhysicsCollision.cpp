#include <QCoreApplication>
#include <iostream>
#include <cmath>
#include "entity/Player.h"
#include "physics/PhysicsEngine.h"
#include "util/Config.h"

static bool close(float a, float b, float eps = 0.001f) {
    return std::fabs(a - b) < eps;
}

#define FAIL(msg) do { std::cerr << "FAIL: " << msg << std::endl; return 1; } while(0)
#define PASS()    do { std::cout << "  PASS" << std::endl; } while(0)

// --- massRatio boundary ---

static int testMassRatioBoundary() {
    std::cout << "testMassRatioBoundary" << std::endl;

    Player a; a.name = "A"; a.id = 1; a.cells[0].pos = {0, 0}; a.cells[0].mass = 110;  // bigger
    Player b; b.name = "B"; b.id = 2; b.cells[0].pos = {0, 0}; b.cells[0].mass = 100;  // 110 > 100*1.1=110 → false, should NOT eat
    b.cells[0].invincibleTimer = 0;

    PhysicsEngine pe;
    QVector<KillResult> kills;
    int count = pe.checkPlayerCollision(a, b, kills);
    if (count != 0) FAIL("110 mass should NOT eat 100 mass (ratio 1.1, not > 1.1)");

    a.cells[0].mass = 111;
    count = pe.checkPlayerCollision(a, b, kills);
    if (count != 1) FAIL("111 mass should eat 100 mass (ratio > 1.1)");

    PASS();
    return 0;
}

// --- invincibleTimer immunity ---

static int testInvincibleImmunity() {
    std::cout << "testInvincibleImmunity" << std::endl;

    Player a; a.name = "A"; a.id = 1; a.cells[0].pos = {0, 0}; a.cells[0].mass = 200;
    Player b; b.name = "B"; b.id = 2; b.cells[0].pos = {0, 0}; b.cells[0].mass = 10;
    b.cells[0].invincibleTimer = 2.0f;

    PhysicsEngine pe;
    QVector<KillResult> kills;
    int count = pe.checkPlayerCollision(a, b, kills);
    if (count != 0) FAIL("invincible player should not be eatable");

    b.cells[0].invincibleTimer = 0.0f;
    count = pe.checkPlayerCollision(a, b, kills);
    if (count != 1) FAIL("non-invincible player should be eatable");

    PASS();
    return 0;
}

// --- team immunity ---

static int testTeamImmunity() {
    std::cout << "testTeamImmunity" << std::endl;

    Player a; a.name = "A"; a.id = 1; a.team = 1; a.cells[0].pos = {0, 0}; a.cells[0].mass = 200;
    Player b; b.name = "B"; b.id = 2; b.team = 1; b.cells[0].pos = {0, 0}; b.cells[0].mass = 10;

    PhysicsEngine pe;
    QVector<KillResult> kills;
    int count = pe.checkPlayerCollision(a, b, kills);
    if (count != 0) FAIL("same team should be immune");

    b.team = 2;
    count = pe.checkPlayerCollision(a, b, kills);
    if (count != 1) FAIL("different team should be eatable");

    PASS();
    return 0;
}

// --- maxMassPerCell cap ---

static int testMaxMassCap() {
    std::cout << "testMaxMassCap" << std::endl;

    Player a; a.name = "A"; a.id = 1;
    a.cells[0].pos = {0, 0};
    a.cells[0].mass = Config::instance().maxMassPerCell * 0.5f;
    Player b; b.name = "B"; b.id = 2;
    b.cells[0].pos = {0, 0};
    b.cells[0].mass = Config::instance().maxMassPerCell; // eating this should cap
    b.cells[0].invincibleTimer = 0;

    PhysicsEngine pe;
    QVector<KillResult> kills;
    pe.checkPlayerCollision(a, b, kills);

    float cap = Config::instance().maxMassPerCell;
    if (a.cells[0].mass > cap + 0.001f) FAIL("cell mass should be capped at maxMassPerCell");

    PASS();
    return 0;
}

// --- overlap threshold ---

static int testOverlapThreshold() {
    std::cout << "testOverlapThreshold" << std::endl;

    Player a; a.name = "A"; a.id = 1; a.cells[0].mass = 200;
    Player b; b.name = "B"; b.id = 2; b.cells[0].mass = 10;
    b.cells[0].invincibleTimer = 0;

    float rA = a.cells[0].radius();
    float rB = b.cells[0].radius();
    // Place B right at edge of A — barely overlapping
    b.cells[0].pos = {rA - rB * 0.4f, 0};
    // d = rA - rB*0.4, overlap = rB*0.4, threshold = rB*0.5 → should NOT eat

    PhysicsEngine pe;
    QVector<KillResult> kills;
    int count = pe.checkPlayerCollision(a, b, kills);
    if (count != 0) FAIL("shallow overlap should NOT trigger eat");

    // Now deep overlap
    b.cells[0].pos = {0, 0};
    count = pe.checkPlayerCollision(a, b, kills);
    if (count != 1) FAIL("full overlap should trigger eat");

    PASS();
    return 0;
}

int main(int argc, char* argv[]) {
    QCoreApplication app(argc, argv);
    int failures = 0;

    if (testMassRatioBoundary() != 0) failures++;
    if (testInvincibleImmunity() != 0) failures++;
    if (testTeamImmunity() != 0) failures++;
    if (testMaxMassCap() != 0) failures++;
    if (testOverlapThreshold() != 0) failures++;

    if (failures == 0) {
        std::cout << "All tests passed!" << std::endl;
        return 0;
    }
    std::cerr << failures << " test(s) failed." << std::endl;
    return 1;
}
