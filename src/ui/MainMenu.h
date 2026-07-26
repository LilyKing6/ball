#ifndef MAINMENU_H
#define MAINMENU_H

#include <QWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QTimer>
#include <QVector>
#include <QProgressBar>

// 远景漂浮球装饰
struct MenuBall {
    QPointF pos;
    QPointF vel;
    float radius;
    QColor color;
    float alpha;   // 透明度系数 (0.3~1.0)
};

// 闪烁星点
struct Star {
    QPointF pos;
    float size;
    float baseAlpha;
    float phase;
    float speed;
    QColor color;      // 支持彩色星点
};

class MainMenu : public QWidget {
    Q_OBJECT
public:
    explicit MainMenu(QWidget* parent = nullptr);

    QString playerName() const { return m_nameInput->text(); }
    void refreshRank();
    void refreshLayout();

signals:
    void showModeSelect();
    void openSettings();
    void showRecords();
    void showRank();
    void showAchievements();

protected:
    void paintEvent(QPaintEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;

private:
    // 左侧面板组件
    QWidget* m_leftPanel = nullptr;
    QLabel* m_avatarLabel = nullptr;
    QLineEdit* m_nameInput = nullptr;
    QLabel* m_tierLabel = nullptr;
    QLabel* m_eloLabel = nullptr;
    QProgressBar* m_eloProgress = nullptr;
    QLabel* m_progressText = nullptr;
    QLabel* m_seasonLabel = nullptr;

    // 右侧区域组件
    QWidget* m_rightPanel = nullptr;
    QLabel* m_titleLabel = nullptr;
    QLabel* m_subtitleLabel = nullptr;
    QPushButton* m_startBtn = nullptr;
    QPushButton* m_settingsBtn = nullptr;
    QPushButton* m_recordsBtn = nullptr;
    QPushButton* m_rankBtn = nullptr;
    QPushButton* m_achievementBtn = nullptr;

    // 动画数据
    QTimer* m_animTimer = nullptr;
    QVector<MenuBall> m_balls;
    QVector<Star> m_stars;
    float m_time = 0.0f;
    float m_pulsePhase = 0.0f;
    bool m_layoutInitialized = false;

    void initStars();
    void initBalls();
    void updateBalls();
    void drawBackground(QPainter& p);
    void drawStars(QPainter& p);
    void drawTitle(QPainter& p);

    void setupLeftPanel();
    void setupRightPanel();
};

#endif // MAINMENU_H
