#ifndef MINIMAPRENDERER_H
#define MINIMAPRENDERER_H

#include <QPainter>
#include <QRect>

class World;

class MinimapRenderer {
public:
    void render(QPainter& p, const QRect& widgetRect, const World& world);

private:
    QPointF worldToMinimap(float wx, float wy, const QRect& minimapRect, float worldW, float worldH) const;
};

#endif // MINIMAPRENDERER_H
