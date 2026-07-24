#ifndef SKINMANAGER_H
#define SKINMANAGER_H

#include <QString>
#include <QColor>
#include <QVector>
#include <QPainter>
#include <QPointF>

enum class SkinType { Solid, Gradient, Striped, Dotted };

struct SkinDef {
    QString id;
    QString name;
    SkinType type;
    QColor primaryColor;
    QColor secondaryColor;
    float patternScale = 1.0f;
};

class SkinManager {
public:
    static SkinManager& instance();

    void loadDefaults();
    const SkinDef& getSkin(const QString& id) const;
    const QVector<SkinDef>& allSkins() const { return m_skins; }

    void applySkin(QPainter& p, const SkinDef& skin, const QPointF& center, float radius);

private:
    SkinManager() = default;
    QVector<SkinDef> m_skins;
};

#endif // SKINMANAGER_H
