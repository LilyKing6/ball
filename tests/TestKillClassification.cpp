#include <QCoreApplication>
#include <iostream>
#include <cmath>
#include "entity/Player.h"
#include "physics/PhysicsEngine.h"

static bool close(float a, float b, float eps = 0.001f) {
    return std::fabs(a - b) < eps;
}

#define FAIL(msg) do { std::cerr << "FAIL: " << msg << std::endl; return 1; } while(0)

static int testSplitKill() {
    Player a;
    a.name = "Killer";
    a.id = 1;
    a.cells[0].pos = {0.0f, 0.0f};
    a.cells[0].mass = 200.0f;
    a.splitTimer = 1.0f;

    Player b;
    b.name = "Victim";
    b.id = 2;
    b.cells[0].pos = {10.0f, 0.0f};
    b.cells[0].mass = 10.0f;
    b.cells[0].invincibleTimer = 0.0f;

    PhysicsEngine physics;
    QVector<KillResult> kills;
    int count = physics.checkPlayerCollision(a, b, kills);

    if (count != 1) {
        std::cerr << "FAIL: expected 1 kill, got " << count << std::endl;
        return 1;
    }
    if (!kills[0].isSplitKill) {
        FAIL("kill should be classified as split kill");
    }
    if (kills[0].isVirusKill) {
        FAIL("kill should not be virus kill");
    }
    return 0;
}

static int testVirusKill() {
    Player a;
    a.name = "Killer";
    a.id = 1;
    a.cells[0].pos = {0.0f, 0.0f};
    a.cells[0].mass = 200.0f;

    Player b;
    b.name = "Victim";
    b.id = 2;
    b.cells[0].pos = {10.0f, 0.0f};
    b.cells[0].mass = 10.0f;
    b.cells[0].invincibleTimer = 0.0f;
    b.virusHitTimer = 1.0f;

    PhysicsEngine physics;
    QVector<KillResult> kills;
    int count = physics.checkPlayerCollision(a, b, kills);

    if (count != 1) {
        std::cerr << "FAIL: expected 1 kill, got " << count << std::endl;
        return 1;
    }
    if (kills[0].isSplitKill) {
        FAIL("kill should not be split kill");
    }
    if (!kills[0].isVirusKill) {
        FAIL("kill should be classified as virus kill");
    }
    return 0;
}

static int testNormalKill() {
    Player a;
    a.name = "Killer";
    a.id = 1;
    a.cells[0].pos = {0.0f, 0.0f};
    a.cells[0].mass = 200.0f;

    Player b;
    b.name = "Victim";
    b.id = 2;
    b.cells[0].pos = {10.0f, 0.0f};
    b.cells[0].mass = 10.0f;
    b.cells[0].invincibleTimer = 0.0f;

    PhysicsEngine physics;
    QVector<KillResult> kills;
    int count = physics.checkPlayerCollision(a, b, kills);

    if (count != 1) {
        std::cerr << "FAIL: expected 1 kill, got " << count << std::endl;
        return 1;
    }
    if (kills[0].isSplitKill || kills[0].isVirusKill) {
        FAIL("normal kill should not have special flags");
    }
    return 0;
}

int main(int argc, char* argv[]) {
    QCoreApplication app(argc, argv);

    int failures = 0;
    failures += testSplitKill();
    failures += testVirusKill();
    failures += testNormalKill();

    if (failures == 0) {
        std::cout << "All kill classification tests passed" << std::endl;
        return 0;
    }
    std::cerr << failures << " kill classification test(s) failed" << std::endl;
    return 1;
}
