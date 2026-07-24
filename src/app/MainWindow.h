#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "engine/GameMode.h"

class GameEngine;
class GLWidget;
class HUDOverlay;
class MainMenu;
class SubWindowManager;
class SettingsWindow;
class RecordsWindow;
class RankWindow;
class AchievementWindow;
class GameOverScreen;
class LeaderboardWidget;
class ModeSelectWindow;
class DebugPanel;

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;

    void setEngine(GameEngine* engine);

public slots:
    void showMainMenu();
    void startGame(GameMode mode);
    void openSettings();
    void openRecords();
    void openRank();
    void openAchievements();
    void openModeSelect();
    void onGameEnded();
    void playAgain();
    void onBackToMenu();
    void applyDisplaySettings();
    void startNetworkedGame(const QString& host, int port, const QString& name);

protected:
    void resizeEvent(QResizeEvent* event) override;

private:
    GLWidget* m_glWidget = nullptr;
    HUDOverlay* m_hud = nullptr;
    MainMenu* m_menu = nullptr;
    SubWindowManager* m_subWindowMgr = nullptr;
    SettingsWindow* m_settingsWindow = nullptr;
    RecordsWindow* m_recordsWindow = nullptr;
    RankWindow* m_rankWindow = nullptr;
    AchievementWindow* m_achievementWindow = nullptr;
    GameOverScreen* m_gameOverScreen = nullptr;
    LeaderboardWidget* m_leaderboard = nullptr;
    ModeSelectWindow* m_modeSelectWindow = nullptr;
    DebugPanel* m_debugPanel = nullptr;
    GameEngine* m_engine = nullptr;
    bool m_returningToMenu = false;

    class NetworkClient* m_netClient = nullptr;
    bool m_networkReconnectFailed = false;
};

#endif // MAINWINDOW_H