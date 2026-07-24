#ifndef MAINMENU_H
#define MAINMENU_H

#include <QWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QTimer>
#include <QVector>
#include <QProgressBar>

// 漂浮球（远景装饰）
struct MenuBall {
    QPointF pos;
    QPointF vel;
    float radius;
    QColor color;
};

// 星点
struct Star {
    QPointF pos;
    float size;
    float baseAlpha;
    float phase;      // 闪烁相位偏移
    float speed;      // 闪烁速度
    QColor color;
};

// 流星
struct Meteor {
    QPointF start;
    QPointF end;
    QPointF current;
    float progress;   // 0.0 → 1.0
    float length;
    float duration;
    bool active;
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
    QTimer* m_meteorTimer = nullptr;
    QVector<MenuBall> m_balls;
    QVector<Star> m_stars;
    QVector<Meteor> m_meteors;
    float m_time = 0.0f;
    bool m_layoutInitialized = false;

    // 初始化方法
    void initStars();
    void initBalls();
    void initMeteor();

    // 更新方法
    void updateStars(float dt);
    void updateMeteors(float dt);
    void updateBalls();

    // 绘制方法
    void drawStars(QPainter& p);
    void drawMeteors(QPainter& p);
    void drawTitle(QPainter& p);

    // 布局方法
    void setupLeftPanel();
    void setupRightPanel();
};

#endif // MAINMENU_H
