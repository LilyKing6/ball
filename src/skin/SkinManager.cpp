#include "SkinManager.h"
#include <QRadialGradient>
#include <QPainterPath>
#include <QtMath>

SkinManager& SkinManager::instance() {
    static SkinManager s;
    return s;
}

void SkinManager::loadDefaults() {
    m_skins.clear();
    m_skins.append({"default", "默认", SkinType::Solid, QColor(80, 140, 220), QColor(40, 80, 160)});
    m_skins.append({"fire", "烈焰", SkinType::Gradient, QColor(255, 80, 20), QColor(255, 200, 20)});
    m_skins.append({"ocean", "海洋", SkinType::Gradient, QColor(20, 120, 220), QColor(20, 220, 200)});
    m_skins.append({"forest", "森林", SkinType::Gradient, QColor(20, 180, 60), QColor(10, 80, 20)});
    m_skins.append({"tiger", "虎纹", SkinType::Striped, QColor(255, 150, 20), QColor(20, 20, 20), 0.3f});
    m_skins.append({"dots", "波点", SkinType::Dotted, QColor(180, 100, 220), QColor(255, 255, 255), 0.25f});
    m_skins.append({"gold", "黄金", SkinType::Solid, QColor(255, 215, 0), QColor(180, 150, 0)});
    m_skins.append({"rose", "玫瑰", SkinType::Gradient, QColor(255, 50, 100), QColor(200, 20, 80)});
}

const SkinDef& SkinManager::getSkin(const QString& id) const {
    for (auto& s : m_skins) {
        if (s.id == id) return s;
    }
    return m_skins[0];
}

void SkinManager::applySkin(QPainter& p, const SkinDef& skin, const QPointF& center, float radius) {
    p.save();
    QPainterPath clipPath;
    clipPath.addEllipse(center, radius, radius);
    p.setClipPath(clipPath);

    switch (skin.type) {
    case SkinType::Solid: {
        QRadialGradient grad(center, radius);
        grad.setColorAt(0, skin.primaryColor.lighter(130));
        grad.setColorAt(0.7, skin.primaryColor);
        grad.setColorAt(1, skin.secondaryColor);
        p.setBrush(grad);
        p.setPen(QPen(skin.secondaryColor.darker(150), 2));
        p.drawEllipse(center, radius, radius);
        break;
    }
    case SkinType::Gradient: {
        QRadialGradient grad(center, radius);
        grad.setColorAt(0, skin.primaryColor.lighter(120));
        grad.setColorAt(0.6, skin.primaryColor);
        grad.setColorAt(1, skin.secondaryColor);
        p.setBrush(grad);
        p.setPen(QPen(skin.secondaryColor.darker(130), 2));
        p.drawEllipse(center, radius, radius);
        break;
    }
    case SkinType::Striped: {
        // Base fill
        p.setBrush(skin.primaryColor);
        p.setPen(Qt::NoPen);
        p.drawEllipse(center, radius, radius);

        // Draw stripes
        float stripeWidth = radius * skin.patternScale * 2.0f;
        if (stripeWidth < 4.0f) stripeWidth = 4.0f;
        p.setBrush(skin.secondaryColor);
        float left = center.x() - radius;
        float right = center.x() + radius;
        float top = center.y() - radius;
        float bottom = center.y() + radius;
        for (float x = left - stripeWidth; x < right + stripeWidth; x += stripeWidth * 2.0f) {
            p.drawRect(QRectF(x, top, stripeWidth, bottom - top));
        }

        // Outline
        p.setBrush(Qt::NoBrush);
        p.setPen(QPen(skin.secondaryColor.darker(130), 2));
        p.drawEllipse(center, radius, radius);
        break;
    }
    case SkinType::Dotted: {
        // Base fill
        p.setBrush(skin.primaryColor);
        p.setPen(Qt::NoPen);
        p.drawEllipse(center, radius, radius);

        // Draw dots in a grid pattern
        float dotSpacing = radius * skin.patternScale * 3.0f;
        if (dotSpacing < 6.0f) dotSpacing = 6.0f;
        float dotR = dotSpacing * 0.3f;
        p.setBrush(skin.secondaryColor);
        for (float dx = -radius; dx <= radius; dx += dotSpacing) {
            float halfChord = qSqrt(radius * radius - dx * dx);
            for (float dy = -halfChord; dy <= halfChord; dy += dotSpacing) {
                p.drawEllipse(QPointF(center.x() + dx, center.y() + dy), dotR, dotR);
            }
        }

        // Outline
        p.setBrush(Qt::NoBrush);
        p.setPen(QPen(skin.secondaryColor.darker(130), 2));
        p.drawEllipse(center, radius, radius);
        break;
    }
    }

    p.restore();
}
