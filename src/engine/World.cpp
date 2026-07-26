#include "World.h"
#include "entity/Spawner.h"
#include "util/Config.h"
#include "util/Random.h"
#include "audio/AudioManager.h"

World::World() {}

void World::init(GameMode mode) {
    auto& cfg = Config::instance();
    auto modeCfg = getModeConfig(mode);
    m_modeCfg = modeCfg;
    m_width = modeCfg.worldWidth;
    m_height = modeCfg.worldHeight;
    m_currentMode = mode;

    m_players.clear();
    m_foods.clear();
    m_spores.clear();
    m_viruses.clear();
    m_bigBeans.clear();
    m_aiControllers.clear();
    m_respawnQueue.clear();
    m_nextPlayerId = 1000;

    // Initialize BattleRoyale safe zone
    if (mode == GameMode::BattleRoyale) {
        m_safeZoneCenter = {m_width / 2, m_height / 2};
        m_safeZoneRadius = qMin(m_width, m_height) / 2;
        m_shrinkTimer = 0;
        m_shrinkPhase = 0;
        // 最小半径 = 初始 * factor^phases
        m_minSafeZoneRadius = m_safeZoneRadius;
        for (int i = 0; i < modeCfg.brShrinkPhases; i++) {
            m_minSafeZoneRadius *= modeCfg.brShrinkFactor;
        }
    }

    Player p;
    p.name = "You";
    p.id = 1;
    p.team = m_modeCfg.hasTeams ? 1 : 0;
    p.isAI = false;
    // 应用模式倍率
    p.speedMul = m_modeCfg.speedMultiplier;
    p.splitVelocityMul = m_modeCfg.splitVelocityMul;
    p.splitCooldownMul = m_modeCfg.splitCooldownMul;
    p.mergeCooldownMul = m_modeCfg.mergeCooldownMul;
    p.cells[0].pos = {randFloat(100, m_width - 100), randFloat(100, m_height - 100)};
    p.cells[0].color = QColor::fromHsv(randInt(0, 359), 200, 230);
    p.cells[0].invincibleTimer = 3.0f;  // 开局 3 秒无敌
    m_players.append(p);
    setLocalPlayer(0);

    for (int i = 0; i < m_modeCfg.aiCount; i++) {
        Player ai;
        if (m_modeCfg.hasTeams) {
            ai.name = QString("TeamA-%1").arg(i + 1);
        } else {
            ai.name = QString("Bot-%1").arg(i + 1);
        }
        ai.id = 100 + i;
        ai.team = m_modeCfg.hasTeams ? 1 : 0;
        ai.isAI = true;
        ai.speedMul = m_modeCfg.speedMultiplier;
        ai.splitVelocityMul = m_modeCfg.splitVelocityMul;
        ai.splitCooldownMul = m_modeCfg.splitCooldownMul;
        ai.mergeCooldownMul = m_modeCfg.mergeCooldownMul;
        ai.cells[0].pos = {randFloat(100, m_width - 100), randFloat(100, m_height - 100)};
        ai.cells[0].color = QColor::fromHsv(randInt(0, 359), 200, 230);
        m_players.append(ai);

        AIDifficulty diff;
        float roll = randFloat(0.0f, 1.0f);
        if (roll < 0.4f)       diff = AIDifficulty::Easy;
        else if (roll < 0.8f)  diff = AIDifficulty::Normal;
        else                   diff = AIDifficulty::Hard;

        m_aiControllers.append(AIController(diff));
    }

    // Team B for team mode
    if (m_modeCfg.hasTeams) {
        for (int i = 0; i < m_modeCfg.aiCount; i++) {
            Player ai;
            ai.name = QString("TeamB-%1").arg(i + 1);
            ai.id = 200 + i;
            ai.team = 2;
            ai.isAI = true;
            ai.speedMul = m_modeCfg.speedMultiplier;
            ai.splitVelocityMul = m_modeCfg.splitVelocityMul;
            ai.splitCooldownMul = m_modeCfg.splitCooldownMul;
            ai.mergeCooldownMul = m_modeCfg.mergeCooldownMul;
            ai.cells[0].pos = {randFloat(100, m_width - 100), randFloat(100, m_height - 100)};
            ai.cells[0].color = QColor::fromHsv(randInt(0, 359), 200, 230);
            m_players.append(ai);

            AIDifficulty diff;
            float roll = randFloat(0.0f, 1.0f);
            if (roll < 0.3f)       diff = AIDifficulty::Easy;
            else if (roll < 0.7f)  diff = AIDifficulty::Normal;
            else                   diff = AIDifficulty::Hard;

            m_aiControllers.append(AIController(diff));
        }
    }

    Spawner::spawnFood(m_foods, m_modeCfg.foodCount, m_width, m_height);

    // Virus spawning: ~12 in 3-4 clusters + rest scattered
    int virusTotal = m_modeCfg.virusCount;
    int clusterCount = 3 + randInt(0, 1);  // 3 or 4 clusters
    int clusteredCount = 12;
    int scatteredCount = virusTotal - clusteredCount;

    for (int ci = 0; ci < clusterCount; ci++) {
        float cx = randFloat(500, m_width - 500);
        float cy = randFloat(500, m_height - 500);
        int perCluster = clusteredCount / clusterCount;
        if (ci == 0) perCluster += clusteredCount % clusterCount;
        for (int vi = 0; vi < perCluster; vi++) {
            float ox = randFloat(-120, 120);
            float oy = randFloat(-120, 120);
            Vec2 cpos(cx + ox, cy + oy);
            cpos.x = clamp(cpos.x, 200.0f, m_width - 200.0f);
            cpos.y = clamp(cpos.y, 200.0f, m_height - 200.0f);
            Virus v(cpos);
            m_viruses.append(v);
        }
    }

    for (int i = 0; i < scatteredCount; i++) {
        Virus v({randFloat(200, m_width - 200), randFloat(200, m_height - 200)});
        m_viruses.append(v);
    }

    // Spawn big colorful beans
    for (int i = 0; i < cfg.bigBeanCount; i++) {
        BigBean bb;
        bb.pos = {randFloat(100, m_width - 100), randFloat(100, m_height - 100)};
        bb.mass = randFloat(cfg.bigBeanMinMass, cfg.bigBeanMaxMass);
        bb.color = QColor::fromHsv(randInt(0, 359), 220, 255);
        m_bigBeans.append(bb);
    }
}

void World::update(float dt) {
    // 网络模式跳过本地物理 —— 由 applySnapshot 覆盖
    if (Config::instance().networkMode) return;

    auto& cfg = Config::instance();
    m_gameTime += dt;

    // Process respawn queue
    for (int i = m_respawnQueue.size() - 1; i >= 0; i--) {
        m_respawnQueue[i].timer -= dt;
        if (m_respawnQueue[i].timer <= 0) {
            auto& entry = m_respawnQueue[i];
            Player np;
            np.name = entry.name;
            np.id = entry.playerId;
            np.team = entry.team;
            np.isAI = entry.isAI;
            // 复活后保留模式倍率
            np.speedMul = m_modeCfg.speedMultiplier;
            np.splitVelocityMul = m_modeCfg.splitVelocityMul;
            np.splitCooldownMul = m_modeCfg.splitCooldownMul;
            np.mergeCooldownMul = m_modeCfg.mergeCooldownMul;
            // 复活：盾归 0
            np.shieldCount = 0;
            np.shieldDecayTimer = 0;
            np.cells[0].pos = {randFloat(100, m_width - 100), randFloat(100, m_height - 100)};
            np.cells[0].color = QColor::fromHsv(randInt(0, 359), 200, 230);
            np.cells[0].invincibleTimer = 3.0f;  // 出生 3 秒无敌
            m_players.append(np);

            // 复活后，把 localPlayer 直接换到新位置：把新追加的 np 和当前 m_players[m_localPlayerIdx] 交换
            // 这样 local 索引保持不变（通常是 0），AI 索引映射也保持不变
            if (!entry.isAI) {
                int newIdx = m_players.size() - 1;
                if (m_localPlayerIdx >= 0 && m_localPlayerIdx != newIdx) {
                    std::swap(m_players[m_localPlayerIdx], m_players[newIdx]);
                    // 交换后 local 仍在 m_localPlayerIdx 位置，无需 setLocalPlayer
                } else {
                    setLocalPlayer(newIdx);
                }
            }

            m_respawnQueue.removeAt(i);
        }
    }

    // AI update - 使用 player.isAI 字段判断，避免依赖 m_localPlayerIdx 时序
    int aiIdx = 0;
    for (int i = 0; i < m_players.size() && aiIdx < m_aiControllers.size(); i++) {
        if (!m_players[i].isAI) continue;  // 跳过本地玩家（通过身份字段）
        m_aiControllers[aiIdx].update(m_players[i], dt, *this);
        aiIdx++;
    }

    m_physics.updatePlayers(m_players, dt, m_width, m_height);

    for (auto& p : m_players) {
        m_frameFoodEaten += m_physics.checkFoodCollision(p, m_foods);
    }

    // Big bean collisions
    for (auto& p : m_players) {
        m_physics.checkBigBeanCollision(p, m_bigBeans);
    }

    // Player vs player devouring
    for (int i = 0; i < m_players.size(); i++) {
        for (int j = i + 1; j < m_players.size(); j++) {
            QVector<KillResult> kills;
            int killCount = m_physics.checkPlayerCollision(m_players[i], m_players[j], kills);
            if (killCount > 0) {
                AudioManager::instance().playSfx("kill");
                for (auto& kr : kills) {
                    m_frameKills.append({kr.victimName, kr.victimMass, kr.isSplitKill, kr.isVirusKill});
                }
            }
        }
    }

    // Player eats spores
    for (auto& p : m_players) {
        for (auto& em : m_spores) {
            if (!em.alive) continue;
            if (em.ownerId == p.id && em.immunityTimer > 0) continue;
            for (auto& c : p.cells) {
                if (!c.alive) continue;
                float d = (c.pos - em.pos).length();
                if (d < c.radius()) {
                    c.mass += em.mass;
                    if (c.mass > cfg.maxMassPerCell) c.mass = cfg.maxMassPerCell;
                    em.alive = false;
                }
            }
        }
    }

    for (auto& em : m_spores) {
        if (em.immunityTimer > 0) em.immunityTimer -= dt;
        em.update(dt);
        // Clamp spore 到可拾取范围：cell 半径最大约 200，spore 半径约 22
        // 确保 spore 中心始终在 [margin, m_width - margin] 内，玩家能够到
        float sporeMargin = em.radius() + 5.0f;
        em.pos.x = clamp(em.pos.x, sporeMargin, m_width - sporeMargin);
        em.pos.y = clamp(em.pos.y, sporeMargin, m_height - sporeMargin);
    }
    m_spores.erase(std::remove_if(m_spores.begin(), m_spores.end(),
        [](const Spore& em) { return !em.alive; }), m_spores.end());

    // Virus collisions - use index-based loop to avoid UB when appending
    QVector<Virus> newViruses;
    for (int vi = 0; vi < m_viruses.size(); vi++) {
        auto& v = m_viruses[vi];
        if (!v.alive) continue;

        // Spore hits virus
        for (auto& em : m_spores) {
            if (!em.alive) continue;
            float d = (em.pos - v.pos).length();
            if (d < v.radius() + em.radius()) {
                Vec2 dir = (v.pos - em.pos).normalized();
                v.pos += dir * 30.0f;
                v.mass += em.mass;
                em.alive = false;

                if (v.mass > cfg.virusSplitThreshold) {
                    Virus newV;
                    newV.pos = v.pos + Vec2(randFloat(-50, 50), randFloat(-50, 50));
                    newV.mass = cfg.virusMass;
                    v.mass = cfg.virusMass;
                    newViruses.append(newV);
                }
            }
        }

        // Player cells hit virus
        for (auto& p : m_players) {
            for (auto& c : p.cells) {
                if (!c.alive) continue;
                float dist = (c.pos - v.pos).length();
                if (dist < c.radius() + v.radius()) {
                    if (c.mass > v.mass * 1.1f) {
                        // 吃刺球获得 +1 防护盾（仅在大逃杀模式中有意义，其他模式也保留以备扩展）
                        if (m_currentMode == GameMode::BattleRoyale) {
                            p.shieldCount += 1;
                        }
                        // Player eats virus: gain random mass, then split into 9 fragments
                        float virusMass = v.mass;
                        float bonusMass = randFloat(0.0f, 50.0f);
                        c.mass += virusMass + bonusMass;
                        if (c.mass > cfg.maxMassPerCell) c.mass = cfg.maxMassPerCell;

                        p.virusHitTimer = 2.0f;

                        int fragmentCount = cfg.virusFragmentCount;
                        int maxNewCells = cfg.maxCellsPerPlayer - p.cells.size();
                        if (maxNewCells <= 0) {
                            // Already at max cells, just gain mass
                            v.alive = false;
                            AudioManager::instance().playSfx("virus_hit");
                            continue;
                        }
                        fragmentCount = qMin(fragmentCount, maxNewCells + 1); // +1 for the original cell

                        float totalMass = c.mass;
                        float perFragmentMass = totalMass / fragmentCount;
                        c.mass = perFragmentMass;
                        c.isMerging = true;
                        c.mergeTimer = cfg.mergeCooldown;

                        for (int fi = 1; fi < fragmentCount; fi++) {
                            Cell newCell;
                            newCell.mass = perFragmentMass;
                            float angle = (fi * 2.0f * 3.14159f) / (fragmentCount - 1);
                            newCell.pos = c.pos + Vec2(cos(angle), sin(angle)) * (c.radius() * 3);
                            newCell.color = c.color;
                            newCell.splitCooldown = 0.5f;
                            newCell.vel = Vec2(cos(angle), sin(angle)) * cfg.virusFragmentVelocity;
                            newCell.isMerging = true;
                            newCell.mergeTimer = cfg.mergeCooldown;
                            p.cells.append(newCell);
                        }

                        v.alive = false;
                        AudioManager::instance().playSfx("virus_hit");
                    }
                }
            }
        }
    }
    m_viruses.append(newViruses);

    // Virus respawn
    for (auto& v : m_viruses) {
        if (!v.alive) {
            v.respawnTimer -= cfg.fixedDt;
            if (v.respawnTimer <= 0) {
                v.pos = {randFloat(200, m_width - 200), randFloat(200, m_height - 200)};
                v.mass = cfg.virusMass;
                v.alive = true;
                v.respawnTimer = cfg.virusRespawnTime;
            }
        }
    }

    Spawner::respawnFood(m_foods, cfg.foodCount, m_width, m_height);

    // Clean up dead players and queue respawns
    for (int i = m_players.size() - 1; i >= 0; i--) {
        bool allDead = std::all_of(m_players[i].cells.begin(), m_players[i].cells.end(),
            [](const Cell& c) { return !c.alive; });
        if (m_players[i].cells.isEmpty() || allDead) {
            // 使用 player.isAI 字段判断身份，避免索引混淆
            bool isAI = m_players[i].isAI;
            bool isLocal = !isAI;

            if (isLocal) {
                AudioManager::instance().playSfx("death");
                m_localPlayerDied = true;
            }

            // Queue respawn
            RespawnEntry entry;
            entry.playerId = m_players[i].id;
            entry.name = m_players[i].name;
            entry.timer = cfg.respawnDelay;
            entry.isAI = isAI;
            entry.team = m_players[i].team;
            if (isAI) {
                entry.difficulty = AIDifficulty::Normal;
            }
            m_respawnQueue.append(entry);

            // 计算这个 AI 玩家对应的 AIController 索引：
            // 在 [0..i-1] 范围内统计有多少个 isAI=true 的 player
            int aiCtrlIdx = -1;
            if (isAI) {
                aiCtrlIdx = 0;
                for (int k = 0; k < i; k++) {
                    if (m_players[k].isAI) aiCtrlIdx++;
                }
            }

            // 调整 local index
            if (i < m_localPlayerIdx) m_localPlayerIdx--;
            else if (i == m_localPlayerIdx) m_localPlayerIdx = -1;

            m_players.removeAt(i);
            if (isAI && aiCtrlIdx >= 0 && aiCtrlIdx < m_aiControllers.size()) {
                m_aiControllers.removeAt(aiCtrlIdx);
            }
        }
    }

    // Check win conditions
    // BattleRoyale: 只有缩圈结束后才判定"最后一人"，避免开局就获胜
    if (m_currentMode == GameMode::BattleRoyale) {
        if (m_shrinkPhase >= m_modeCfg.brShrinkPhases &&
            m_players.size() == 1 && m_localPlayerIdx >= 0) {
            m_localPlayerWon = true;
            qDebug() << "Local player won BattleRoyale!";
        }
    }

    // TeamMode: 不再"全员阵亡"提前判定（玩家会复活）
    // 胜利判定改为时限到达由 GameEngine 比较双方质量

    // BattleRoyale shrinking zone - 阶梯式缩圈
    if (m_currentMode == GameMode::BattleRoyale) {
        m_shrinkTimer += dt;
        if (m_shrinkPhase < m_modeCfg.brShrinkPhases &&
            m_shrinkTimer >= m_modeCfg.brShrinkInterval) {
            m_shrinkTimer = 0;
            m_safeZoneRadius *= m_modeCfg.brShrinkFactor;
            if (m_safeZoneRadius < m_minSafeZoneRadius) m_safeZoneRadius = m_minSafeZoneRadius;
            m_shrinkPhase++;
            qDebug() << "BR shrink phase" << m_shrinkPhase << "of" << m_modeCfg.brShrinkPhases
                     << "radius=" << m_safeZoneRadius;
        }

        // 圈外伤害逻辑：先扣盾，盾破后按比例扣质量，质量到 0 时 cell 死亡
        const float shieldDecayPeriod = 5.0f;  // 5秒消耗一个盾
        const float massDecayRate = 0.05f;     // 盾破后每秒扣 5% 质量
        const float minDecayPerSec = 30.0f;    // 保底每秒至少扣 30 质量（避免小球磨不死）

        for (auto& p : m_players) {
            // 统计圈外的存活分身数
            int outsideCount = 0;
            for (auto& c : p.cells) {
                if (!c.alive) continue;
                float d = (c.pos - m_safeZoneCenter).length();
                if (d > m_safeZoneRadius) outsideCount++;
            }

            if (outsideCount == 0) {
                // 全部在圈内：重置盾计时器
                p.shieldDecayTimer = 0.0f;
                continue;
            }

            // 圈外有分身：推进盾计时器
            if (p.shieldCount > 0) {
                p.shieldDecayTimer += dt;
                if (p.shieldDecayTimer >= shieldDecayPeriod) {
                    p.shieldDecayTimer -= shieldDecayPeriod;
                    int deduct = qMin(outsideCount, p.shieldCount);
                    p.shieldCount -= deduct;
                }
            } else {
                // 盾已耗尽：圈外分身按 max(5%/s, 30/s) 扣质量
                p.shieldDecayTimer = 0.0f;
                for (auto& c : p.cells) {
                    if (!c.alive) continue;
                    float d = (c.pos - m_safeZoneCenter).length();
                    if (d > m_safeZoneRadius) {
                        // 比例扣血 + 保底扣血，取大者
                        float pctLoss = c.mass * massDecayRate * dt;
                        float minLoss = minDecayPerSec * dt;
                        float loss = qMax(pctLoss, minLoss);
                        c.mass -= loss;
                        if (c.mass < 1.0f) {
                            c.mass = 0;
                            c.alive = false;  // 质量耗尽，分身死亡
                        }
                    }
                }
            }
        }
    }
}

// ============================================================================
// 网络模式：用服务端 snapshot 覆盖 World 状态
// ============================================================================
#include "WorldSnapshot.h"
#include "util/Random.h"

void World::applySnapshot(const WorldSnapshot& snap, int myPlayerId) {
    // 1. 同步世界尺寸
    m_width = snap.worldWidth;
    m_height = snap.worldHeight;

    // 2. 清空所有实体（网络模式以服务端为权威）
    m_players.clear();
    m_foods.clear();
    m_viruses.clear();
    m_bigBeans.clear();
    m_spores.clear();
    m_aiControllers.clear();
    m_respawnQueue.clear();
    m_localPlayerIdx = -1;

    // 3. 重建玩家
    for (const auto& po : snap.players) {
        Player p;
        p.id = po.id;
        p.name = po.name;
        p.team = po.team;
        p.shieldCount = po.shieldCount;
        p.isAI = (po.id != myPlayerId);
        p.cells.clear();
        // cells 重建
        for (const auto& co : po.cells) {
            Cell c;
            c.pos = {co.x, co.y};
            c.mass = co.mass;
            c.alive = true;
            // 稳定颜色（按 id 哈希），不随 snapshot 闪烁
            c.color = QColor::fromHsv((po.id * 47 + 13) % 360, 200, 230);
            p.cells.append(c);
        }
        // 同步游标到质心（本地玩家由 GLWidget 每帧覆盖）
        p.virtualCursor = p.centerOfMass();
        p.mouseWorldPos = p.virtualCursor;

        if (po.id == myPlayerId) {
            m_localPlayerIdx = m_players.size();
        }
        m_players.append(p);
    }

    // 4. 重建食物
    for (const auto& fo : snap.foods) {
        Food f;
        f.pos = {fo.x, fo.y};
        f.mass = Config::instance().foodMass;
        // 颜色由位置哈希生成，避免每帧抖动
        int hue = (qAbs(int(fo.x * 7 + fo.y * 13)) % 360);
        f.color = QColor::fromHsv(hue, 200, 230);
        f.alive = true;
        m_foods.append(f);
    }

    // 5. 重建病毒
    for (const auto& vo : snap.viruses) {
        Virus v({vo.x, vo.y});
        v.mass = Config::instance().virusMass;
        v.alive = true;
        m_viruses.append(v);
    }

    // 6. 重建孢子
    for (const auto& so : snap.spores) {
        Spore s;
        s.pos = {so.x, so.y};
        s.mass = so.mass;
        s.alive = true;
        m_spores.append(s);
    }

    // 7. 重建大豆
    for (const auto& bo : snap.bigBeans) {
        BigBean bb;
        bb.pos = {bo.x, bo.y};
        bb.mass = bo.mass;
        bb.alive = true;
        bb.color = QColor::fromHsv((static_cast<int>(bo.x * 7 + bo.y * 13) % 360), 220, 255);
        m_bigBeans.append(bb);
    }

    // 8. 大逃杀字段同步
    m_safeZoneRadius = snap.safeZoneRadius;
    m_safeZoneCenter = {snap.safeZoneCenterX, snap.safeZoneCenterY};
    m_shrinkPhase = snap.shrinkPhase;
    m_gameTime = snap.gameTime;
}


Vec2 World::clampPosition(const Vec2& pos) const {
    return {clamp(pos.x, 0.0f, m_width), clamp(pos.y, 0.0f, m_height)};
}

bool World::isInBounds(const Vec2& pos) const {
    return pos.x >= 0 && pos.x <= m_width && pos.y >= 0 && pos.y <= m_height;
}

float World::localRespawnTimer() const {
    for (const auto& entry : m_respawnQueue) {
        if (!entry.isAI) return entry.timer;
    }
    return -1.0f;
}

float World::timeToNextShrink() const {
    if (m_currentMode != GameMode::BattleRoyale) return -1.0f;
    if (m_shrinkPhase >= m_modeCfg.brShrinkPhases) return -1.0f;
    return m_modeCfg.brShrinkInterval - m_shrinkTimer;
}

bool World::inShrinkWarning() const {
    if (m_currentMode != GameMode::BattleRoyale) return false;
    if (m_shrinkPhase >= m_modeCfg.brShrinkPhases) return false;
    return timeToNextShrink() <= m_modeCfg.brShrinkWarnTime;
}

float World::teamAMass() const {
    float total = 0;
    for (const auto& p : m_players) {
        if (p.team == 1) total += p.totalMass();
    }
    return total;
}

float World::teamBMass() const {
    float total = 0;
    for (const auto& p : m_players) {
        if (p.team == 2) total += p.totalMass();
    }
    return total;
}
