#ifndef HUDOVERLAY_H
#define HUDOVERLAY_H

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QPropertyAnimation>
#include <QTimer>

class HUDSettingsPanel;
class LeaderboardWidget;
class World;

class HUDOverlay : public QWidget {
    Q_OBJECT
    Q_PROPERTY(float toastOpacity READ toastOpacity WRITE setToastOpacity)
    Q_PROPERTY(float massPulse READ massPulse WRITE setMassPulse)
    Q_PROPERTY(float poisonOpacity READ poisonOpacity WRITE setPoisonOpacity)
public:
    explicit HUDOverlay(QWidget* parent = nullptr);

    void setMass(float mass);
    void setShield(int count);     // 防护盾数量
    void setKills(int kills);
    void setPoison(float remaining);  // 剩余中毒秒数,0 = 无中毒
    void setRank(int rank, int total);
    void setMode(const QString& mode);
    void setTimeRemaining(int seconds);
    void setTeamScores(float teamA, float teamB);
    void setZoneWarning(const QString& text);
    void showDeathOverlay(float respawnSeconds);
    void hideDeathOverlay();

    void showAchievement(const QString& name, const QString& description);
    bool isToastActive() const { return m_toastActive; }

    float toastOpacity() const { return m_toastOpacity; }
    void setToastOpacity(float v);

    float massPulse() const { return m_massPulse; }
    void setMassPulse(float v);

    float poisonOpacity() const { return m_poisonOpacity; }
    void setPoisonOpacity(float v);

    void setLeaderboard(LeaderboardWidget* lb);
    void setGameWidget(QWidget* w) { m_gameWidget = w; }
    bool minimapVisible() const { return m_minimapVisible; }
    void setMinimapVisible(bool v);

    // 显式显示/隐藏所有 HUD 元素
    void showHUD();
    void hideHUD();

signals:
    void backToMenu();
    void openSettings();
    void quitGame();
    void minimapVisibilityChanged(bool visible);

protected:
    void resizeEvent(QResizeEvent* event) override;
    // 鼠标事件转发给 GLWidget，让游戏控制正常工作
    void mouseMoveEvent(QMouseEvent* e) override;
    void mousePressEvent(QMouseEvent* e) override;
    void mouseReleaseEvent(QMouseEvent* e) override;
    void wheelEvent(QWheelEvent* e) override;

private:
    // === 顶部横条 ===
    QWidget* m_topBar = nullptr;
    QLabel* m_massLabel = nullptr;
    QLabel* m_shieldLabel = nullptr;        // 防护盾显示
    QLabel* m_timerLabel = nullptr;
    QLabel* m_zoneWarningLabel = nullptr;   // 缩圈警告
    QPushButton* m_leaderboardBtn = nullptr;
    QPushButton* m_minimapBtn = nullptr;
    QPushButton* m_settingsBtn = nullptr;
    QPushButton* m_backBtn = nullptr;

    // === 悬浮技能按钮 ===
    QWidget* m_skillPanel = nullptr;
    QPushButton* m_splitBtn = nullptr;
    QPushButton* m_ejectBtn = nullptr;

    // === 设置弹窗 ===
    HUDSettingsPanel* m_settingsPanel = nullptr;

    // === 外部组件引用 ===
    LeaderboardWidget* m_leaderboard = nullptr;
    QWidget* m_gameWidget = nullptr;
    bool m_minimapVisible = true;
    bool m_hudVisible = false;

    // === 死亡遮罩 ===
    QLabel* m_deathLabel = nullptr;

    // === Toast ===
    QLabel* m_toastLabel = nullptr;
    QPropertyAnimation* m_toastAnim = nullptr;
    QTimer* m_toastTimer = nullptr;
    float m_toastOpacity = 0.0f;
    bool m_toastActive = false;

    // === 质量脉冲动画 ===
    float m_massPulse = 0.0f;
    float m_prevMass = 0.0f;
    QPropertyAnimation* m_pulseAnim = nullptr;

    // === 中毒边缘脉冲 ===
    QWidget* m_poisonOverlay = nullptr;
    QPropertyAnimation* m_poisonAnim = nullptr;
    float m_poisonOpacity = 0.0f;
    bool m_poisonActive = false;
    float m_poisonRemaining = 0.0f;

    // === 布局 ===
    void createTopBar();
    void createSkillButtons();
    void createSettingsPanel();
    void updatePositions();
    QColor massColor(float mass) const;
    int skillButtonSize() const;
    void forwardMouseEvent(QMouseEvent* e);
};

#endif // HUDOVERLAY_H
