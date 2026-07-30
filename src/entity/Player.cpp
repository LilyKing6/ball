#include "Player.h"
#include "util/Config.h"
#include "util/Random.h"

Cell::Cell() {
    mass = Config::instance().initialMass;
    color = QColor::fromHsv(randInt(0, 359), 200, 230);
}

float Cell::radius() const {
    return Config::instance().radiusConstant * qSqrt(mass);
}

Player::Player() {
    cells.append(Cell{});
}

float Player::totalMass() const {
    float sum = 0;
    for (auto& c : cells) sum += c.mass;
    return sum;
}

Vec2 Player::centerOfMass() const {
    Vec2 com;
    float total = 0;
    for (auto& c : cells) {
        com += c.pos * c.mass;
        total += c.mass;
    }
    return total > 0 ? com / total : com;
}

bool Player::canSplit() const {
    auto& cfg = Config::instance();
    if (cells.size() >= cfg.maxCellsPerPlayer) return false;
    for (auto& c : cells) {
        if (c.mass >= cfg.splitMinMass && c.splitCooldown <= 0) return true;
    }
    return false;
}

void Player::split(Vec2 direction, QVector<Cell>& newCells) {
    auto& cfg = Config::instance();
    if (!canSplit()) return;

    // 标记分裂窗口，用于成就统计
    splitTimer = 1.5f;

    float effSplitVel = cfg.splitVelocity * splitVelocityMul;
    float effSplitCD = cfg.splitCooldown * splitCooldownMul;
    float effMergeCD = cfg.mergeCooldown * mergeCooldownMul;

    QVector<Cell> splitCells;
    for (auto& c : cells) {
        if (!c.alive || c.mass < cfg.splitMinMass || c.splitCooldown > 0) continue;
        if (cells.size() + newCells.size() >= cfg.maxCellsPerPlayer) break;

        float halfMass = c.mass / 2.0f;
        c.mass = halfMass;

        Cell newCell;
        newCell.mass = halfMass;
        newCell.pos = c.pos + direction * (c.radius() + newCell.radius() + 10);
        newCell.color = c.color;
        newCell.splitCooldown = effSplitCD;
        newCell.vel = direction * effSplitVel;
        newCell.isMerging = true;
        newCell.mergeTimer = effMergeCD;

        c.isMerging = true;
        c.mergeTimer = effMergeCD;

        newCells.append(newCell);
    }
}

void Player::eject(Vec2 direction, QVector<Spore>& ejected) {
    auto& cfg = Config::instance();
    if (m_ejectCooldown > 0) return;

    Vec2 globalDir = direction;
    float globalDirLen = globalDir.length();

    for (auto& c : cells) {
        if (!c.alive || c.mass <= cfg.ejectMass + 10.0f) continue;

        // 每个 cell 独立计算朝游标方向（聚球时朝同一目标位置）
        // direction 来自 GameEngine：unit dir from centerOfMass to virtualCursor
        // 聚球：每个 cell 朝虚拟游标点吐
        Vec2 cellDir = globalDir;
        // 如果 direction 是单位向量，全体朝同方向（普通吐球）
        // 如果是绝对位置（virtualCursor），则每个 cell 计算自己的方向
        // 由调用方决定（GameEngine 的 ejectFromLocalPlayer 已传入单位向量）

        c.mass -= cfg.ejectMass;
        Spore em;
        float sporeR = Config::instance().radiusConstant * qSqrt(cfg.ejectMass);
        em.pos = c.pos + cellDir * (c.radius() + sporeR + 10.0f);
        em.vel = cellDir * cfg.ejectVelocity;
        em.mass = cfg.ejectMass;
        em.ownerId = id;
        em.immunityTimer = 0.05f;  // 短免疫避免立即被吞回
        ejected.append(em);
    }
    m_ejectCooldown = cfg.ejectCooldown;
}

// 聚球：所有 cell 朝指定的世界目标位置吐 spore
// 每个 cell 各自计算朝向，使 spore 飞向目标，可被目标 cell 吃掉聚球
void Player::ejectToward(Vec2 worldTarget, QVector<Spore>& ejected) {
    auto& cfg = Config::instance();
    if (m_ejectCooldown > 0) return;

    for (auto& c : cells) {
        if (!c.alive || c.mass <= cfg.ejectMass + 10.0f) continue;

        Vec2 toTarget = worldTarget - c.pos;
        float len = toTarget.length();
        if (len < 0.001f) continue;
        Vec2 dir = toTarget / len;

        c.mass -= cfg.ejectMass;
        Spore em;
        float sporeR = Config::instance().radiusConstant * qSqrt(cfg.ejectMass);
        em.pos = c.pos + dir * (c.radius() + sporeR + 10.0f);
        em.vel = dir * cfg.ejectVelocity;
        em.mass = cfg.ejectMass;
        em.ownerId = id;
        em.immunityTimer = 0.05f;
        ejected.append(em);
    }
    m_ejectCooldown = cfg.ejectCooldown;
}

void Player::update(float dt) {
    auto& cfg = Config::instance();

    if (splitTimer > 0.0f) {
        splitTimer -= dt;
        if (splitTimer < 0.0f) splitTimer = 0.0f;
    }
    if (virusHitTimer > 0.0f) {
        virusHitTimer -= dt;
        if (virusHitTimer < 0.0f) virusHitTimer = 0.0f;
    }

    m_ejectCooldown -= dt;

    // 合并冷却倒计时
    for (int i = 0; i < cells.size(); i++) {
        if (!cells[i].alive || !cells[i].isMerging) continue;
        cells[i].mergeTimer -= dt;
        if (cells[i].mergeTimer <= 0) {
            cells[i].isMerging = false;
        }
    }

    // Cell 间的相互作用：冷却期内排斥，冷却结束后允许接近
    for (int i = 0; i < cells.size(); i++) {
        for (int j = i + 1; j < cells.size(); j++) {
            if (!cells[i].alive || !cells[j].alive) continue;

            Vec2 diff = cells[i].pos - cells[j].pos;
            float dist = diff.length();
            float r1 = cells[i].radius();
            float r2 = cells[j].radius();
            float minDist = r1 + r2;

            // 零距离 / 完全重叠时直接合并（跳过冷却检查）
            if (dist < 0.001f) {
                if (!cells[i].isMerging && !cells[j].isMerging) {
                    float totalMass = cells[i].mass + cells[j].mass;
                    cells[i].mass = totalMass;
                    if (cells[i].mass > cfg.maxMassPerCell) cells[i].mass = cfg.maxMassPerCell;
                    cells[j].alive = false;
                }
                continue;
            }

            // 冷却期内：弹性推开，避免穿插和瞬时融合
            if ((cells[i].isMerging || cells[j].isMerging) && dist < minDist) {
                float overlap = minDist - dist;
                Vec2 normal = diff / dist;
                float pushFactor = 0.3f;
                cells[i].pos += normal * overlap * pushFactor * 0.5f;
                cells[j].pos -= normal * overlap * pushFactor * 0.5f;
                continue;
            }

            // 冷却完成：允许融合（接近完全重叠时）
            if (!cells[i].isMerging && !cells[j].isMerging) {
                float overlap = minDist - dist;
                float minRadius = qMin(r1, r2);

                // 阈值 0.85 - 必须充分重叠
                if (overlap >= minRadius * 0.85f) {
                    // 动量守恒
                    float m1 = cells[i].mass;
                    float m2 = cells[j].mass;
                    float totalMass = m1 + m2;
                    Vec2 mergedVel = (cells[i].vel * m1 + cells[j].vel * m2) / totalMass;
                    Vec2 mergedPos = (cells[i].pos * m1 + cells[j].pos * m2) / totalMass;

                    cells[i].mass = totalMass;
                    if (cells[i].mass > cfg.maxMassPerCell) cells[i].mass = cfg.maxMassPerCell;
                    cells[i].pos = mergedPos;
                    cells[i].vel = mergedVel;
                    cells[j].alive = false;
                }
            }
        }
    }

    cells.erase(std::remove_if(cells.begin(), cells.end(), [](const Cell& c) { return !c.alive; }), cells.end());

    for (auto& c : cells) {
        if (!c.alive) continue;

        // 出生无敌倒计时
        if (c.invincibleTimer > 0) {
            c.invincibleTimer -= dt;
            if (c.invincibleTimer < 0) c.invincibleTimer = 0;
        }

        if (c.splitCooldown > 0) {
            c.splitCooldown -= dt;
            float speed = c.vel.length();
            if (speed > 50.0f) {
                c.vel *= 0.95f;
            }
        }

        // 中毒：每帧损失质量
        if (c.poisonTimer > 0.0f) {
            c.poisonTimer -= dt;
            if (c.poisonTimer < 0.0f) c.poisonTimer = 0.0f;
            c.mass -= 8.0f * dt * 60.0f;
            if (c.mass < 5.0f) c.mass = 5.0f;
        }

        // 使用虚拟游标驱动移动
        float dist = (virtualCursor - c.pos).length();
        float deadZone = c.radius() * 0.5f;
        // 应用模式速度倍率
        // 速度软化：mass > 400 后不再按 sqrt 减速，改用开立方曲线 + 保底下限
        float effectiveMass = qMin(c.mass, 400.0f);
        float maxSpeed = (cfg.baseSpeed * speedMul) / qSqrt(effectiveMass);
        // 大质量保底最低速度（mass > 1000 时不会完全动不了）
        float minSpeed = 25.0f * speedMul;
        if (c.mass > 400.0f) {
            float largeSpeed = (cfg.baseSpeed * speedMul) / qPow(c.mass, 1.0f/3.0f);
            maxSpeed = qMax(largeSpeed, minSpeed);
        }

        if (dist < deadZone) {
            c.vel = Vec2{0, 0};
        } else {
            float speedFactor = clamp((dist - deadZone) / (deadZone * 3.0f), 0.0f, 1.0f);
            speedFactor = easeOutCubic(speedFactor);
            c.vel = (virtualCursor - c.pos).normalized() * maxSpeed * speedFactor;
        }

        c.update(dt);
    }
}
