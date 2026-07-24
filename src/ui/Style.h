#ifndef STYLE_H
#define STYLE_H

#include <QString>
#include <QColor>
#include <QPushButton>
#include <QFrame>

// 统一视觉规范：颜色、圆角、间距、字体大小、常用组件样式
namespace Style {

// ========== 颜色 ==========
QColor bgDark();          // 主背景深色
QColor bgPanel();         // 面板背景
QColor bgPanelLight();    // 面板亮部
QColor accentGold();      // 金色强调
QColor accentBlue();      // 蓝色强调
QColor accentRed();       // 红色强调
QColor accentGreen();     // 绿色强调
QColor textPrimary();     // 主文字
QColor textSecondary();   // 次要文字
QColor textMuted();       // 弱化文字
QColor border();          // 边框

// ========== 常用尺寸 ==========
int borderRadiusSmall();  // 8
int borderRadiusMedium(); // 12
int borderRadiusLarge();  // 20
int borderRadiusRound();  // 999 (胶囊)

// ========== 按钮样式 ==========
// 主按钮（渐变，大）
QString primaryButtonStyle();
// 次要按钮（描边，半透明）
QString secondaryButtonStyle();
// 危险/退出按钮
QString dangerButtonStyle();
// 图标小按钮
QString iconButtonStyle();

void applyPrimaryButton(QPushButton* btn);
void applySecondaryButton(QPushButton* btn);
void applyDangerButton(QPushButton* btn);
void applyIconButton(QPushButton* btn, const QString& color = "#FFD700");

// ========== 卡片/面板样式 ==========
QString cardStyle();
QString glassPanelStyle();
QString sectionTitleStyle();
QString labelStyle(const QColor& color, int pixelSize, bool bold = false);

// ========== 输入框/下拉框/滑块样式 ==========
QString lineEditStyle();
QString comboBoxStyle();
QString sliderStyle(const QColor& handleColor);
QString listWidgetStyle();

// ========== 标题艺术字绘制 ==========
// 在指定位置绘制带渐变填充+发光+描边的标题文字
void drawGradientTitle(QPainter& p, const QString& text, const QRect& rect,
                       const QList<QColor>& colors, int fontSize = 48);

// ========== 帮助函数 ==========
QString rgba(const QColor& c, int alpha);
QString rgbaF(const QColor& c, float alpha);

} // namespace Style

#endif // STYLE_H
