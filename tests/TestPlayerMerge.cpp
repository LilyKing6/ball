#include <QCoreApplication>
#include <iostream>
#include <cmath>
#include "entity/Player.h"
#include "util/Config.h"

static bool close(float a, float b, float eps = 0.001f) {
    return std::fabs(a - b) < eps;
}

#define FAIL(msg) do { std::cerr << "FAIL: " << msg << std::endl; return 1; } while(0)
#define PASS()    do { std::cout << "  PASS" << std::endl; } while(0)

// --- merge cooldown prevents merge ---

static int testMergeCooldownBlocks() {
    std::cout << "testMergeCooldownBlocks" << std::endl;

    Player p;
    p.name = "Test";
    p.id = 1;
    p.cells[0].pos = {0, 0};
    p.cells[0].mass = 50;
    p.cells[0].isMerging = true;
    p.cells[0].mergeTimer = Config::instance().mergeCooldown;

    Cell c2;
    c2.pos = {0, 0}; // same position
    c2.mass = 50;
    c2.isMerging = true;
    c2.mergeTimer = Config::instance().mergeCooldown;
    p.cells.append(c2);

    // Run update with both in cooldown — they should NOT merge
    float dt = 1.0f / 120.0f;
    for (int i = 0; i < 10; i++) {
        p.update(dt);
    }

    if (p.cells.size() != 2) FAIL("cells should NOT merge during cooldown");

    PASS();
    return 0;
}

// --- merge after cooldown ---

static int testMergeAfterCooldown() {
    std::cout << "testMergeAfterCooldown" << std::endl;

    Player p;
    p.name = "Test";
    p.id = 1;
    p.cells[0].pos = {0, 0};
    p.cells[0].mass = 50;
    p.cells[0].isMerging = true;
    p.cells[0].mergeTimer = Config::instance().mergeCooldown;

    Cell c2;
    c2.pos = {0, 0};
    c2.mass = 50;
    c2.isMerging = true;
    c2.mergeTimer = Config::instance().mergeCooldown;
    p.cells.append(c2);

    // Simulate full cooldown pass
    float dt = 1.0f / 120.0f;
    float totalDt = Config::instance().mergeCooldown + 1.0f;
    int steps = static_cast<int>(totalDt / dt);
    for (int i = 0; i < steps; i++) {
        p.update(dt);
    }

    // Should now be merged into one cell
    if (p.cells.size() != 1) FAIL("cells should merge after cooldown");
    if (!close(p.cells[0].mass, 100.0f)) FAIL("merged mass should be 100");

    PASS();
    return 0;
}

// --- momentum conservation on merge ---

static int testMergeMomentum() {
    std::cout << "testMergeMomentum" << std::endl;

    auto& cfg = Config::instance();
    float savedMergeCD = cfg.mergeCooldown;
    cfg.mergeCooldown = 0.0f; // no cooldown for this test

    Player p;
    p.name = "Test";
    p.id = 1;
    p.cells[0].pos = {0, 0};
    p.cells[0].mass = 100;
    p.cells[0].vel = {10, 0};
    p.cells[0].isMerging = false;

    Cell c2;
    c2.pos = {0, 0};
    c2.mass = 100;
    c2.vel = {-10, 0};
    c2.isMerging = false;
    p.cells.append(c2);

    // Run several frames to allow merge
    float dt = 1.0f / 120.0f;
    for (int i = 0; i < 60; i++) {
        p.update(dt);
    }

    // Merged velocity should be near 0 (equal mass opposite direction)
    if (p.cells.size() != 1) FAIL("cells should merge");
    if (!close(p.cells[0].vel.x, 0.0f, 0.1f)) FAIL("momentum should be conserved (vx ~ 0)");
    if (!close(p.cells[0].mass, 200.0f)) FAIL("merged mass should be 200");

    cfg.mergeCooldown = savedMergeCD;
    PASS();
    return 0;
}

// --- speed curve: large mass still movable ---

static int testSpeedCurveSoftening() {
    std::cout << "testSpeedCurveSoftening" << std::endl;

    Player p;
    p.name = "Big";
    p.id = 1;
    p.speedMul = 1.0f;
    p.cells[0].pos = {0, 0};
    p.cells[0].mass = 5000;  // very large
    p.virtualCursor = {1000, 0};  // far away cursor
    p.cells[0].splitCooldown = -1; // make sure we don't split

    float dt = 1.0f / 120.0f;
    p.update(dt);

    // A 5000-mass cell should still move (not stuck at 0 velocity)
    if (close(p.cells[0].vel.x, 0.0f)) FAIL("large cell should still have movement capability");

    PASS();
    return 0;
}

int main(int argc, char* argv[]) {
    QCoreApplication app(argc, argv);
    int failures = 0;

    if (testMergeCooldownBlocks() != 0) failures++;
    if (testMergeAfterCooldown() != 0) failures++;
    if (testMergeMomentum() != 0) failures++;
    if (testSpeedCurveSoftening() != 0) failures++;

    if (failures == 0) {
        std::cout << "All tests passed!" << std::endl;
        return 0;
    }
    std::cerr << failures << " test(s) failed." << std::endl;
    return 1;
}
