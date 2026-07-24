#ifndef SIMPLERENDERER_H
#define SIMPLERENDERER_H

#include <QWidget>
#include <QPainter>
#include <QVector>
#include <QColor>
#include "util/Math.h"

struct SimplePlayer {
    QString name;
    QVector<QPointF> cells;
    QVector<float> masses;
    QColor color;
};

struct SimpleFood {
    QPointF pos;
    QColor color;
};

class SimpleRenderer {
public:
    void render(QWidget* widget, const QVector<SimplePlayer>& players, 
                const QVector<SimpleFood>& foods, const QPointF& cameraPos, float zoom) {
        QPainter p(widget);
        p.setRenderHint(QPainter::Antialiasing);
        
        QSize sz = widget->size();
        float aspect = (float)sz.width() / sz.height();
        
        // Draw background
        p.fillRect(widget->rect(), QColor(26, 26, 46));
        
        // Draw grid
        p.setPen(QColor(60, 60, 90));
        float gridSize = 100.0f * zoom;
        for (float x = 0; x < 6000; x += gridSize) {
            QPointF sp = worldToScreen(x, 3000, cameraPos, zoom, sz, aspect);
            if (sp.x() >= 0 && sp.x() <= sz.width())
                p.drawLine(QPointF(sp.x(), 0), QPointF(sp.x(), sz.height()));
        }
        for (float y = 0; y < 6000; y += gridSize) {
            QPointF sp = worldToScreen(3000, y, cameraPos, zoom, sz, aspect);
            if (sp.y() >= 0 && sp.y() <= sz.height())
                p.drawLine(QPointF(0, sp.y()), QPointF(sz.width(), sp.y()));
        }
        
        // Draw world border
        QRectF border = QRectF(
            worldToScreen(0, 0, cameraPos, zoom, sz, aspect),
            worldToScreen(6000, 6000, cameraPos, zoom, sz, aspect)
        );
        p.setPen(QColor(255, 215, 0));
        p.setBrush(Qt::NoBrush);
        p.drawRect(border);
        
        // Draw foods
        for (const auto& f : foods) {
            QPointF sp = worldToScreen(f.pos.x(), f.pos.y(), cameraPos, zoom, sz, aspect);
            float r = 5.0f * zoom;
            if (sp.x() > -r && sp.x() < sz.width() + r && sp.y() > -r && sp.y() < sz.height() + r) {
                p.setBrush(f.color);
                p.setPen(Qt::NoPen);
                p.drawEllipse(sp, r, r);
            }
        }
        
        // Draw players
        for (const auto& pl : players) {
            for (int i = 0; i < pl.cells.size(); i++) {
                QPointF sp = worldToScreen(pl.cells[i].x(), pl.cells[i].y(), cameraPos, zoom, sz, aspect);
                float r = 4.0f * sqrt(pl.masses[i]) * zoom;
                if (r < 2) r = 2;
                if (sp.x() > -r && sp.x() < sz.width() + r && sp.y() > -r && sp.y() < sz.height() + r) {
                    // Gradient effect
                    QRadialGradient grad(sp, r);
                    grad.setColorAt(0, pl.color.lighter(130));
                    grad.setColorAt(0.7, pl.color);
                    grad.setColorAt(1, pl.color.darker(120));
                    p.setBrush(grad);
                    p.setPen(QColor(0, 0, 0, 80));
                    p.drawEllipse(sp, r, r);
                }
            }
        }
    }
    
private:
    QPointF worldToScreen(float wx, float wy, const QPointF& camPos, float zoom, const QSize& sz, float aspect) {
        float hw = sz.height() * aspect / 2.0f;
        float hh = sz.height() / 2.0f;
        
        float sx = (wx - camPos.x()) * zoom + sz.width() / 2.0f;
        float sy = (wy - camPos.y()) * zoom + sz.height() / 2.0f;
        
        return QPointF(sx, sy);
    }
};

#endif // SIMPLERENDERER_H