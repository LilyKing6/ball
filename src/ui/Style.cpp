#include "Style.h"
#include <QPainter>
#include <QPainterPath>
#include <QFontMetrics>

namespace Style {

QColor bgDark()          { return QColor(12, 12, 28); }
QColor bgPanel()         { return QColor(22, 33, 62); }
QColor bgPanelLight()    { return QColor(32, 44, 78); }
QColor accentGold()      { return QColor(255, 215, 0); }
QColor accentBlue()      { return QColor(79, 195, 247); }
QColor accentRed()       { return QColor(231, 76, 60); }
QColor accentGreen()     { return QColor(46, 204, 113); }
QColor textPrimary()     { return QColor(224, 224, 224); }
QColor textSecondary()   { return QColor(160, 160, 160); }
QColor textMuted()       { return QColor(100, 100, 120); }
QColor border()          { return QColor(255, 255, 255, 30); }

int borderRadiusSmall()  { return 8; }
int borderRadiusMedium() { return 12; }
int borderRadiusLarge()  { return 20; }
int borderRadiusRound()  { return 9999; }

QString rgba(const QColor& c, int alpha) {
    return QString("rgba(%1,%2,%3,%4)").arg(c.red()).arg(c.green()).arg(c.blue()).arg(alpha);
}
QString rgbaF(const QColor& c, float alpha) {
    return QString("rgba(%1,%2,%3,%4)").arg(c.red()).arg(c.green()).arg(c.blue()).arg(alpha, 0, 'f', 2);
}

QString primaryButtonStyle() {
    return R"(
        QPushButton {
            font-size: 18px;
            font-weight: bold;
            color: white;
            background: qlineargradient(x1:0,y1:0,x2:1,y2:0,stop:0 #FF6B6B,stop:0.5 #FFA500,stop:1 #FFD700);
            border: none;
            border-radius: 28px;
            padding: 8px 24px;
        }
        QPushButton:hover {
            background: qlineargradient(x1:0,y1:0,x2:1,y2:0,stop:0 #FF8585,stop:0.5 #FFB733,stop:1 #FFE066);
        }
        QPushButton:pressed {
            background: qlineargradient(x1:0,y1:0,x2:1,y2:0,stop:0 #E05555,stop:0.5 #E69500,stop:1 #E6C200);
        }
    )";
}

QString secondaryButtonStyle() {
    return R"(
        QPushButton {
            font-size: 14px;
            color: #ccc;
            background: rgba(255,255,255,0.06);
            border: 1px solid rgba(255,255,255,0.12);
            border-radius: 14px;
            padding: 6px 16px;
        }
        QPushButton:hover {
            color: white;
            background: rgba(255,255,255,0.12);
            border-color: rgba(255,255,255,0.25);
        }
        QPushButton:pressed {
            background: rgba(255,255,255,0.04);
        }
    )";
}

QString dangerButtonStyle() {
    return R"(
        QPushButton {
            font-size: 16px;
            color: white;
            background: rgba(231,76,60,0.85);
            border: none;
            border-radius: 22px;
            padding: 8px 20px;
        }
        QPushButton:hover {
            background: rgba(255,85,85,0.95);
        }
    )";
}

QString iconButtonStyle() {
    return R"(
        QPushButton {
            color: rgba(255,255,255,0.7);
            background: transparent;
            border: none;
            font-size: 16px;
        }
        QPushButton:hover {
            color: white;
            background: rgba(255,255,255,0.1);
            border-radius: 14px;
        }
    )";
}

void applyPrimaryButton(QPushButton* btn) {
    btn->setCursor(Qt::PointingHandCursor);
    btn->setStyleSheet(primaryButtonStyle());
}
void applySecondaryButton(QPushButton* btn) {
    btn->setCursor(Qt::PointingHandCursor);
    btn->setStyleSheet(secondaryButtonStyle());
}
void applyDangerButton(QPushButton* btn) {
    btn->setCursor(Qt::PointingHandCursor);
    btn->setStyleSheet(dangerButtonStyle());
}
void applyIconButton(QPushButton* btn, const QString& color) {
    btn->setCursor(Qt::PointingHandCursor);
    btn->setStyleSheet(QString(R"(
        QPushButton {
            color: %1;
            background: transparent;
            border: none;
            font-size: 16px;
        }
        QPushButton:hover {
            color: white;
            background: rgba(255,255,255,0.1);
            border-radius: 14px;
        }
    )").arg(color));
}

QString cardStyle() {
    return QString(R"(
        QFrame {
            background: rgba(22,33,62,0.85);
            border: 1px solid rgba(255,255,255,0.08);
            border-radius: %1px;
        }
    )").arg(borderRadiusMedium());
}

QString glassPanelStyle() {
    return QString(R"(
        QWidget {
            background: rgba(15,25,50,0.78);
            border: 1px solid rgba(255,255,255,0.12);
            border-radius: %1px;
        }
    )").arg(borderRadiusMedium());
}

QString sectionTitleStyle() {
    return "color: #FFD700; font-size: 15px; font-weight: bold;";
}

QString labelStyle(const QColor& color, int pixelSize, bool bold) {
    return QString("color: %1; font-size: %2px; %3; background: transparent;")
        .arg(color.name()).arg(pixelSize).arg(bold ? "font-weight: bold" : "");
}

QString lineEditStyle() {
    return R"(
        QLineEdit {
            font-size: 15px;
            color: white;
            background: rgba(255,255,255,0.08);
            border: 1px solid rgba(255,255,255,0.15);
            border-radius: 18px;
            padding: 6px 14px;
        }
        QLineEdit:focus {
            border-color: #FFD700;
            background: rgba(255,255,255,0.12);
        }
    )";
}

QString comboBoxStyle() {
    return R"(
        QComboBox {
            background: rgba(255,255,255,0.08);
            color: #ccc;
            border: 1px solid rgba(255,255,255,0.15);
            border-radius: 8px;
            padding: 6px 12px;
            font-size: 13px;
        }
        QComboBox:hover { border-color: rgba(255,255,255,0.3); }
        QComboBox::drop-down { border: none; width: 24px; }
        QComboBox QAbstractItemView {
            background: #1a1a2e;
            color: #ccc;
            border: 1px solid rgba(255,255,255,0.15);
            selection-background-color: rgba(255,215,0,0.2);
        }
    )";
}

QString sliderStyle(const QColor& handleColor) {
    return QString(R"(
        QSlider::groove:horizontal { height: 6px; background: rgba(255,255,255,0.1); border-radius: 3px; }
        QSlider::sub-page:horizontal { background: %1; border-radius: 3px; }
        QSlider::handle:horizontal { width: 16px; height: 16px; margin: -5px 0; background: white; border-radius: 8px; }
    )").arg(handleColor.name());
}

QString listWidgetStyle() {
    return R"(
        QListWidget {
            background: rgba(0,0,0,0.2);
            border: 1px solid rgba(255,255,255,0.08);
            border-radius: 8px;
            color: #ccc;
            font-size: 13px;
            outline: none;
        }
        QListWidget::item {
            padding: 10px;
            border-bottom: 1px solid rgba(255,255,255,0.05);
            border-radius: 6px;
        }
        QListWidget::item:hover {
            background: rgba(255,255,255,0.06);
        }
        QListWidget::item:selected {
            background: rgba(255,215,0,0.15);
            color: #FFD700;
        }
    )";
}

void drawGradientTitle(QPainter& p, const QString& text, const QRect& rect,
                       const QList<QColor>& colors, int fontSize) {
    if (colors.isEmpty()) return;

    QFont font("Microsoft YaHei", fontSize, QFont::Bold);
    p.setFont(font);
    QFontMetrics fm(font);
    int textWidth = fm.horizontalAdvance(text);
    int textHeight = fm.height();
    int x = rect.x() + (rect.width() - textWidth) / 2;
    int y = rect.y() + (rect.height() + textHeight / 2) / 2;

    QLinearGradient grad(x, y - textHeight, x + textWidth, y);
    for (int i = 0; i < colors.size(); ++i) {
        grad.setColorAt(i / qMax(1.0, colors.size() - 1.0), colors[i]);
    }

    // 多层外发光
    QPainterPath path;
    path.addText(x, y, font, text);
    p.setPen(Qt::NoPen);
    for (int i = 5; i > 0; --i) {
        QColor glow(100, 150, 255, 12 * i);
        p.strokePath(path, QPen(glow, i * 3));
    }

    // 描边
    p.strokePath(path, QPen(QColor(255, 255, 255, 120), 2));

    // 渐变填充
    p.fillPath(path, grad);
}

} // namespace Style
