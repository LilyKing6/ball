#include "WorldSnapshot.h"
#include "engine/World.h"
#include "entity/Player.h"
#include "entity/Food.h"
#include "entity/Virus.h"
#include "entity/Spore.h"
#include "entity/BigBean.h"
#include "engine/GameMode.h"

WorldSnapshot WorldSnapshot::fromWorld(const World& world, int tickId,
                                        float centerX, float centerY, float observerRadius) {
    WorldSnapshot s;
    s.tickId = tickId;
    s.gameTime = world.gameTime();
    s.worldWidth = world.width();
    s.worldHeight = world.height();
    s.gameMode = static_cast<int>(world.currentMode());

    s.safeZoneRadius = world.safeZoneRadius();
    s.safeZoneCenterX = world.safeZoneCenter().x;
    s.safeZoneCenterY = world.safeZoneCenter().y;
    s.shrinkPhase = world.currentShrinkPhase();
    s.timeToNextShrink = world.timeToNextShrink();

    const Player* local = world.localPlayer();
    bool useClipping = (observerRadius > 0);
    float radiusSq = observerRadius * observerRadius;

    auto inRange = [&](float x, float y) -> bool {
        if (!useClipping) return true;
        float dx = x - centerX;
        float dy = y - centerY;
        return (dx * dx + dy * dy) <= radiusSq;
    };

    // 玩家：本地玩家始终包含；其他玩家若 cells 在裁剪范围内才包含
    for (const auto& p : world.players()) {
        bool isLocal = (&p == local);
        PlayerObservation po;
        po.id = p.id;
        po.name = p.name;
        po.team = p.team;
        po.shieldCount = p.shieldCount;
        po.isLocal = isLocal;
        po.isAlive = !p.cells.isEmpty();

        bool anyCellInRange = isLocal;  // local 永远收录
        for (const auto& c : p.cells) {
            if (!c.alive) continue;
            if (!isLocal && useClipping && !inRange(c.pos.x, c.pos.y)) continue;
            CellObservation co;
            co.x = c.pos.x;
            co.y = c.pos.y;
            co.mass = c.mass;
            po.cells.append(co);
            anyCellInRange = true;
        }
        if (anyCellInRange) s.players.append(po);
    }

    // 食物：视野裁剪
    for (const auto& f : world.foods()) {
        if (!f.alive) continue;
        if (useClipping && !inRange(f.pos.x, f.pos.y)) continue;
        FoodObservation fo;
        fo.x = f.pos.x;
        fo.y = f.pos.y;
        s.foods.append(fo);
    }

    // 病毒：视野裁剪
    for (const auto& v : world.viruses()) {
        if (!v.alive) continue;
        if (useClipping && !inRange(v.pos.x, v.pos.y)) continue;
        VirusObservation vo;
        vo.x = v.pos.x;
        vo.y = v.pos.y;
        s.viruses.append(vo);
    }

    // 孢子：视野裁剪
    for (const auto& sp : world.spores()) {
        if (!sp.alive) continue;
        if (useClipping && !inRange(sp.pos.x, sp.pos.y)) continue;
        SporeObservation so;
        so.x = sp.pos.x;
        so.y = sp.pos.y;
        so.mass = sp.mass;
        so.radius = sp.radius();
        s.spores.append(so);
    }

    // 大豆：视野裁剪
    for (const auto& bb : world.bigBeans()) {
        if (!bb.alive) continue;
        if (useClipping && !inRange(bb.pos.x, bb.pos.y)) continue;
        BigBeanObservation bo;
        bo.x = bb.pos.x;
        bo.y = bb.pos.y;
        bo.mass = bb.mass;
        s.bigBeans.append(bo);
    }

    return s;
}
