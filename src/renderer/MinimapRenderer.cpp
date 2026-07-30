#include "MinimapRenderer.h"
#include "engine/World.h"
#include "util/Config.h"
#include <QPainterPath>
#include <algorithm>

QPointF MinimapRenderer::worldToMinimap(float wx, float wy, const QRect& r, float worldW, float worldH) const {
    float sx = r.x() + (wx / worldW) * r.width();
    // Y 轴对齐 OpenGL：世界 Y=0 在屏幕下方（minimap 底部），Y=worldH 在顶部
    float sy = r.y() + r.height() - (wy / worldH) * r.height();
    return QPointF(sx, sy);
}

void MinimapRenderer::render(QPainter& p, const QRect& widgetRect, const World& world) {
    auto& cfg = Config::instance();
    int size = cfg.minimapSize;
    int margin = cfg.minimapMargin;
    int maxPlayers = cfg.minimapMaxPlayers;
    float worldW = world.width();
    float worldH = world.height();

    // 右上角显示（避开顶部 44px 的 HUD 横条）
    int topOffset = 50;
    QRect mmRect(widgetRect.width() - size - margin, topOffset, size, size);

    p.setPen(Qt::NoPen);
    p.setBrush(QColor(0, 0, 0, 150));
    p.drawRoundedRect(mmRect, 8, 8);

    p.setPen(QPen(QColor(255, 255, 255, 80), 1));
    p.setBrush(Qt::NoBrush);
    p.drawRoundedRect(mmRect, 8, 8);

    // World boundary
    QPointF tl = worldToMinimap(0, 0, mmRect, worldW, worldH);
    QPointF br = worldToMinimap(worldW, worldH, mmRect, worldW, worldH);
    p.setPen(QPen(QColor(255, 255, 255, 60), 1));
    p.drawRect(QRectF(tl, br));

    // 安全区只在大逃杀模式显示
    if (world.currentMode() == GameMode::BattleRoyale && world.safeZoneRadius() > 0) {
        QPointF szCenter = worldToMinimap(world.safeZoneCenter().x, world.safeZoneCenter().y, mmRect, worldW, worldH);
        float szR = (world.safeZoneRadius() / worldW) * mmRect.width();
        p.setPen(QPen(QColor(0, 200, 255, 120), 1, Qt::DashLine));
        p.setBrush(QColor(0, 200, 255, 20));
        p.drawEllipse(szCenter, szR, szR);
    }

    // Collect players
    struct PlayerInfo {
        Vec2 pos;
        QColor color;
        float mass;
        bool isLocal;
        int team;
    };
    QVector<PlayerInfo> infos;
    const Player* local = world.localPlayer();

    for (const auto& pl : world.players()) {
        float mass = pl.totalMass();
        if (mass <= 0 || pl.cells.isEmpty()) continue;
        bool isLocal = (&pl == local);
        infos.append({pl.centerOfMass(), pl.cells[0].color, mass, isLocal, pl.team});
    }

    // 团战模式：用团队颜色显示，本地玩家高亮
    if (world.currentMode() == GameMode::TeamMode) {
        for (const auto& info : infos) {
            QPointF sp = worldToMinimap(info.pos.x, info.pos.y, mmRect, worldW, worldH);
            float r = info.isLocal ? 5.0f : 3.0f;
            QColor markerColor = (info.team == 1) ? QColor(80, 150, 255) : QColor(255, 80, 80);
            if (info.isLocal) {
                // 本地玩家额外高亮：白边
                p.setPen(QPen(QColor(255, 255, 255), 2));
            } else {
                p.setPen(QPen(markerColor.darker(150), 1));
            }
            p.setBrush(markerColor);
            p.drawEllipse(sp, r, r);
        }
    } else {
        // 非团战：按质量排序显示前 N 名 + 总是显示本地玩家
        QVector<PlayerInfo> sorted = infos;
        std::sort(sorted.begin(), sorted.end(), [](const PlayerInfo& a, const PlayerInfo& b) {
            return a.mass > b.mass;
        });

        for (int i = 0; i < qMin(sorted.size(), maxPlayers); i++) {
            QPointF sp = worldToMinimap(sorted[i].pos.x, sorted[i].pos.y, mmRect, worldW, worldH);
            float r = 4.0f;

            QColor markerColor;
            if (i == 0) markerColor = QColor(255, 215, 0);      // Gold
            else if (i == 1) markerColor = QColor(192, 192, 192); // Silver
            else markerColor = QColor(205, 127, 50);              // Bronze

            p.setPen(QPen(markerColor.darker(130), 1));
            p.setBrush(markerColor);
            p.drawEllipse(sp, r, r);
        }

        // 总是显示本地玩家：白边 + 自己球的颜色
        if (local && local->totalMass() > 0 && !local->cells.isEmpty()) {
            QPointF sp = worldToMinimap(local->centerOfMass().x, local->centerOfMass().y, mmRect, worldW, worldH);
            float r = 5.0f;
            // 中毒:本地玩家小地图标记边框变紫
            bool localPoisoned = false;
            for (const auto& lc : local->cells) {
                if (lc.poisonTimer > 0.0f) { localPoisoned = true; break; }
            }
            QColor borderColor = localPoisoned ? QColor(180, 0, 255) : QColor(255, 255, 255);
            p.setPen(QPen(borderColor, 2));
            p.setBrush(local->cells[0].color);
            p.drawEllipse(sp, r, r);
        }
    }

    // 本地玩家方向指示：基于 virtualCursor（与实际移动方向一致）
    if (local && local->totalMass() > 0) {
        Vec2 com = local->centerOfMass();
        Vec2 cursorDir = local->virtualCursor - com;
        float dirLen = cursorDir.length();
        if (dirLen > 5.0f) {  // 太短的指针看不清
            cursorDir = cursorDir / dirLen;
            QPointF sp = worldToMinimap(com.x, com.y, mmRect, worldW, worldH);
            // worldToMinimap 已对 Y 翻转：方向向量的 Y 也要翻转以匹配
            QPointF tip = sp + QPointF(cursorDir.x * 10.0f, -cursorDir.y * 10.0f);
            p.setPen(QPen(QColor(255, 230, 80, 220), 2));
            p.drawLine(sp, tip);
            // 箭头尖端
            p.setPen(Qt::NoPen);
            p.setBrush(QColor(255, 230, 80, 220));
            p.drawEllipse(tip, 1.8f, 1.8f);
        }
    }

    // Viruses
    for (const auto& v : world.viruses()) {
        if (!v.alive) continue;
        QPointF sp = worldToMinimap(v.pos.x, v.pos.y, mmRect, worldW, worldH);
        p.setPen(Qt::NoPen);
        p.setBrush(v.color());
        QPainterPath tri;
        tri.moveTo(sp.x(), sp.y() - 4);
        tri.lineTo(sp.x() - 3, sp.y() + 3);
        tri.lineTo(sp.x() + 3, sp.y() + 3);
        tri.closeSubpath();
        p.drawPath(tri);
    }
}
