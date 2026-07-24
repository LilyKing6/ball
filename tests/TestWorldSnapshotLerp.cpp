#include <QCoreApplication>
#include <iostream>
#include <cmath>
#include "engine/WorldSnapshot.h"

static bool close(float a, float b, float eps = 0.001f) {
    return std::fabs(a - b) < eps;
}

#define FAIL(msg) do { std::cerr << "FAIL: " << msg << std::endl; return 1; } while(0)

static int testLerpBasic() {
    WorldSnapshot prev;
    prev.tickId = 1;
    prev.worldWidth = 3000;
    prev.worldHeight = 3000;

    PlayerObservation p;
    p.id = 42;
    p.name = "Tester";
    p.cells.append({100.0f, 200.0f, 10.0f});
    prev.players.append(p);

    WorldSnapshot cur = prev;
    cur.tickId = 2;
    cur.players[0].cells[0].x = 200.0f;
    cur.players[0].cells[0].y = 300.0f;
    cur.players[0].cells[0].mass = 20.0f;

    WorldSnapshot mid = WorldSnapshot::lerp(prev, cur, 0.5f);
    if (mid.players.size() != 1) {
        FAIL("player count after lerp");
    }
    const auto& c = mid.players[0].cells[0];
    if (!close(c.x, 150.0f) || !close(c.y, 250.0f) || !close(c.mass, 15.0f)) {
        std::cerr << "FAIL: interpolation values x=" << c.x << " y=" << c.y << " mass=" << c.mass << std::endl;
        return 1;
    }

    WorldSnapshot at0 = WorldSnapshot::lerp(prev, cur, 0.0f);
    if (!close(at0.players[0].cells[0].x, 100.0f)) {
        FAIL("alpha=0 should equal prev");
    }

    WorldSnapshot at1 = WorldSnapshot::lerp(prev, cur, 1.0f);
    if (!close(at1.players[0].cells[0].x, 200.0f)) {
        FAIL("alpha=1 should equal cur");
    }

    return 0;
}

static int testLerpNewCell() {
    WorldSnapshot prev;
    PlayerObservation p;
    p.id = 7;
    p.cells.append({0.0f, 0.0f, 10.0f});
    prev.players.append(p);

    WorldSnapshot cur;
    PlayerObservation p2;
    p2.id = 7;
    p2.cells.append({100.0f, 0.0f, 10.0f});
    p2.cells.append({0.0f, 100.0f, 5.0f}); // new cell after split
    cur.players.append(p2);

    WorldSnapshot mid = WorldSnapshot::lerp(prev, cur, 0.5f);
    if (mid.players[0].cells.size() != 2) {
        FAIL("new cell should be preserved");
    }
    if (!close(mid.players[0].cells[0].x, 50.0f)) {
        FAIL("existing cell should interpolate");
    }
    if (!close(mid.players[0].cells[1].x, 0.0f) || !close(mid.players[0].cells[1].y, 100.0f)) {
        FAIL("new cell should use cur position");
    }
    return 0;
}

static int testLerpNewPlayer() {
    WorldSnapshot prev;
    WorldSnapshot cur;
    PlayerObservation p;
    p.id = 99;
    p.cells.append({500.0f, 500.0f, 10.0f});
    cur.players.append(p);

    WorldSnapshot mid = WorldSnapshot::lerp(prev, cur, 0.5f);
    if (mid.players.size() != 1 || !close(mid.players[0].cells[0].x, 500.0f)) {
        FAIL("new player should use cur position");
    }
    return 0;
}

int main(int argc, char* argv[]) {
    QCoreApplication app(argc, argv);

    int failures = 0;
    failures += testLerpBasic();
    failures += testLerpNewCell();
    failures += testLerpNewPlayer();

    if (failures == 0) {
        std::cout << "All WorldSnapshot::lerp tests passed" << std::endl;
        return 0;
    }
    std::cerr << failures << " WorldSnapshot::lerp test(s) failed" << std::endl;
    return 1;
}
