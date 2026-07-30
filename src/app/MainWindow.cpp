#include "MainWindow.h"
#include "renderer/GLWidget.h"
#include "ui/HUDOverlay.h"
#include "ui/MainMenu.h"
#include "ui/SubWindowManager.h"
#include "ui/SettingsWindow.h"
#include "ui/RecordsWindow.h"
#include "ui/RankWindow.h"
#include "ui/AchievementWindow.h"
#include "ui/GameOverScreen.h"
#include "ui/LeaderboardWidget.h"
#include "ui/ModeSelectWindow.h"
#include "ui/DebugPanel.h"
#include "achievement/AchievementManager.h"
#include "engine/GameEngine.h"
#include "engine/World.h"
#include "network/NetworkClient.h"
#include "util/Config.h"
#include <QTimer>
#include <QResizeEvent>
#include <QDebug>
#include <QScreen>
#include <QGuiApplication>
#include <QApplication>
#include <QMessageBox>
#include <QUrl>

// 预设分辨率定义
struct ResolutionPreset {
    int width;
    int height;
};

static const ResolutionPreset kResolutions[] = {
    {1280, 720},
    {1920, 1080},
    {2560, 1440},
    {3840, 2160},
};

static const int kResolutionCount = sizeof(kResolutions) / sizeof(kResolutions[0]);

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
    setWindowTitle("球球大作战");

    // 应用保存的显示设置
    auto& cfg = Config::instance();
    int idx = qBound(0, cfg.resolutionIndex, kResolutionCount - 1);
    const auto& res = kResolutions[idx];

    // 锁定窗口大小为预设分辨率，禁止自由拉伸
    setFixedSize(res.width, res.height);

    // 全屏模式延迟到 show 后
    if (cfg.displayMode == 1) {
        setWindowFlags(Qt::Window | Qt::FramelessWindowHint);
    }

    m_glWidget = new GLWidget(this);
    setCentralWidget(m_glWidget);

    // HUD and Leaderboard must be direct children of MainWindow, not GLWidget
    // QOpenGLWidget doesn't support QWidget children properly
    m_hud = new HUDOverlay(this);
    m_hud->hide();

    m_menu = new MainMenu(this);
    m_menu->hide();

    m_subWindowMgr = new SubWindowManager(this);
    m_subWindowMgr->hide();

    m_settingsWindow = new SettingsWindow(this);
    m_recordsWindow = new RecordsWindow(this);
    m_rankWindow = new RankWindow(this);
    m_achievementWindow = new AchievementWindow(this);
    m_gameOverScreen = new GameOverScreen(this);
    m_leaderboard = new LeaderboardWidget(this);
    m_modeSelectWindow = new ModeSelectWindow(this);

    // Debug panel
    m_debugPanel = new DebugPanel(this);
    m_debugPanel->hide();
    m_debugPanel->move(width() - 210, 300);

    connect(m_menu, &MainMenu::showModeSelect, this, &MainWindow::openModeSelect);
    connect(m_menu, &MainMenu::openSettings, this, &MainWindow::openSettings);
    connect(m_menu, &MainMenu::showRecords, this, &MainWindow::openRecords);
    connect(m_menu, &MainMenu::showRank, this, &MainWindow::openRank);
    connect(m_menu, &MainMenu::showAchievements, this, &MainWindow::openAchievements);
    connect(m_hud, &HUDOverlay::backToMenu, this, &MainWindow::onBackToMenu);
    connect(m_hud, &HUDOverlay::openSettings, this, &MainWindow::openSettings);
    connect(m_hud, &HUDOverlay::quitGame, this, []() { QApplication::quit(); });
    connect(m_gameOverScreen, &GameOverScreen::playAgain, this, &MainWindow::playAgain);
    connect(m_gameOverScreen, &GameOverScreen::returnToMenu, this, &MainWindow::showMainMenu);
    connect(m_modeSelectWindow, &ModeSelectWindow::modeSelected, this, &MainWindow::startGame);
    connect(m_modeSelectWindow, &ModeSelectWindow::networkModeSelected,
            this, &MainWindow::startNetworkedGame);
    connect(m_settingsWindow, &SettingsWindow::resolutionChanged, this, &MainWindow::applyDisplaySettings);
    connect(m_settingsWindow, &SettingsWindow::displayModeChanged, this, &MainWindow::applyDisplaySettings);

    // Debug panel toggle
    connect(m_glWidget, &GLWidget::toggleDebugPanel, this, [this]() {
        if (m_debugPanel->isVisible()) {
            m_debugPanel->hide();
        } else {
            m_debugPanel->move(width() - 210, 300);
            m_debugPanel->show();
            m_debugPanel->raise();
        }
    });
}

MainWindow::~MainWindow() {}

void MainWindow::setEngine(GameEngine* engine) {
    m_engine = engine;
    m_glWidget->setEngine(engine);

    m_menu->show();
    m_menu->raise();
    m_menu->resize(width(), height());
    m_menu->refreshLayout();

    connect(m_engine, &GameEngine::gameEnded, this, &MainWindow::onGameEnded);

    m_leaderboard->move(width() - 190, 10);
    m_leaderboard->hide();

    // Wire HUD to leaderboard and game widget
    m_hud->setLeaderboard(m_leaderboard);
    m_hud->setGameWidget(m_glWidget);
    // 同步 HUD 的小地图状态到 GLWidget
    connect(m_hud, &HUDOverlay::minimapVisibilityChanged, m_glWidget, &GLWidget::setShowMinimap);
    m_glWidget->setShowMinimap(m_hud->minimapVisible());

    // Wire debug panel
    m_debugPanel->setEngine(engine);
    connect(m_debugPanel, &DebugPanel::massChanged, engine, &GameEngine::setLocalPlayerMass);
    connect(m_debugPanel, &DebugPanel::godModeToggled, engine, &GameEngine::setGodMode);
    connect(m_debugPanel, &DebugPanel::speedBoostToggled, engine, &GameEngine::setSpeedBoost);
    connect(m_debugPanel, &DebugPanel::spawnFoodRequested, engine, &GameEngine::spawnFoodNearPlayer);
    connect(m_debugPanel, &DebugPanel::spawnBigBeanRequested, engine, &GameEngine::spawnBigBeanNearPlayer);
    connect(m_debugPanel, &DebugPanel::teleportToCenterRequested, engine, &GameEngine::teleportLocalPlayerToCenter);
    connect(m_debugPanel, &DebugPanel::resetRequested, engine, [engine]() {
        engine->setLocalPlayerMass(10);
        engine->setGodMode(false);
        engine->setSpeedBoost(false);
    });

    auto* hudTimer = new QTimer(this);
    connect(hudTimer, &QTimer::timeout, this, [this]() {
        if (!m_engine) return;
        auto* local = m_engine->world().localPlayer();
        if (local) {
            m_hud->setMass(local->totalMass());
            m_hud->setKills(local->kills);
            m_hud->setRank(1, m_engine->world().players().size());
            // 仅在有安全区的模式中显示防护盾
            if (m_engine->currentMode() == GameMode::BattleRoyale) {
                m_hud->setShield(local->shieldCount);
            } else {
                m_hud->setShield(0);
            }
            // 中毒 HUD:取本地玩家所有 cell 的最大 poisonTimer
            float maxPoison = 0.0f;
            for (const auto& c : local->cells) {
                if (c.poisonTimer > maxPoison) maxPoison = c.poisonTimer;
            }
            m_hud->setPoison(maxPoison);
            m_hud->hideDeathOverlay();
        } else {
            m_hud->setShield(0);
            // Player is dead, show respawn countdown
            float respawnTimer = m_engine->world().localRespawnTimer();
            if (respawnTimer > 0) {
                m_hud->showDeathOverlay(respawnTimer);
            }
        }

        // Timer countdown
        auto modeCfg = getModeConfig(m_engine->currentMode());
        if (modeCfg.timeLimitSeconds > 0) {
            int remaining = modeCfg.timeLimitSeconds - m_engine->world().gameTime();
            if (remaining < 0) remaining = 0;
            m_hud->setTimeRemaining(remaining);
        } else {
            m_hud->setTimeRemaining(-1);
        }

        // Team scores
        if (m_engine->currentMode() == GameMode::TeamMode) {
            float teamA = 0, teamB = 0;
            for (const auto& p : m_engine->world().players()) {
                if (p.team == 1) teamA += p.totalMass();
                else if (p.team == 2) teamB += p.totalMass();
            }
            m_hud->setTeamScores(teamA, teamB);
        } else {
            m_hud->setTeamScores(-1, -1); // hide
        }

        // BR zone warning - 使用 World 的状态
        if (m_engine->currentMode() == GameMode::BattleRoyale && local) {
            auto& w = m_engine->world();
            float dist = (local->centerOfMass() - w.safeZoneCenter()).length();
            if (dist > w.safeZoneRadius()) {
                if (local->shieldCount > 0) {
                    m_hud->setZoneWarning(QString("⚠ 圈外！消耗护盾保护中（%1 盾）").arg(local->shieldCount));
                } else {
                    m_hud->setZoneWarning("⚠ 危险区！正在损失质量！");
                }
            } else if (w.inShrinkWarning()) {
                int sec = static_cast<int>(w.timeToNextShrink()) + 1;
                m_hud->setZoneWarning(QString("⚠ 安全区将在 %1 秒后缩小！").arg(sec));
            } else if (dist > w.safeZoneRadius() * 0.85f) {
                m_hud->setZoneWarning("接近边缘！");
            } else {
                m_hud->setZoneWarning("");
            }
        } else {
            m_hud->setZoneWarning("");
        }

        // Check for achievement notifications
        auto& ach = AchievementManager::instance();
        if (!m_hud->isToastActive() && ach.hasPendingNotifications()) {
            auto def = ach.popNotification();
            m_hud->showAchievement(def.name, def.description);
        }
        // Update leaderboard
        if (m_leaderboard->isVisible()) {
            m_leaderboard->refresh(m_engine->world());
        }

        // Sync minimap visibility from HUD to GL widget
        m_glWidget->setShowMinimap(m_hud->minimapVisible());
    });
    hudTimer->start(100);
}

void MainWindow::resizeEvent(QResizeEvent* e) {
    QMainWindow::resizeEvent(e);
    if (m_menu && m_menu->isVisible()) m_menu->resize(e->size());
    if (m_subWindowMgr) m_subWindowMgr->resize(e->size());
    if (m_hud) {
        // HUDOverlay is just a controller, don't let it cover the GL widget
        m_hud->resize(1, 1);
        m_hud->move(0, 0);
    }
    if (m_leaderboard && m_leaderboard->isVisible()) {
        m_leaderboard->move(width() - 190, 10);
        m_leaderboard->raise();
    }
}

void MainWindow::showMainMenu() {
    m_hud->hide();
    m_hud->hideHUD();
    m_leaderboard->hide();
    m_subWindowMgr->clearAll();

    // 清理网络模式残留
    m_networkReconnectFailed = false;
    if (m_netClient) {
        m_netClient->closeConnection();
        delete m_netClient;
        m_netClient = nullptr;
    }
    if (m_glWidget) {
        m_glWidget->setNetworkClient(nullptr);
        m_glWidget->stopNetworkInputLoop();
        m_glWidget->hideReconnectOverlay();
        m_glWidget->setCursor(Qt::ArrowCursor);
    }
    if (m_engine) {
        m_engine->setNetworkMode(false);
        m_engine->setNetworkClient(nullptr);
    }
    Config::instance().networkMode = false;

    m_menu->show();
    m_menu->raise();
    m_menu->resize(width(), height());
    m_menu->refreshLayout();
    m_menu->refreshRank();
}

void MainWindow::onBackToMenu() {
    m_returningToMenu = true;
    if (m_engine && m_engine->state().current() == GameStateEnum::Playing) {
        m_engine->endGame();
    }
    showMainMenu();
}

void MainWindow::startGame(GameMode mode) {
    // 清理网络模式残留
    auto& cfg = Config::instance();
    cfg.networkMode = false;
    m_engine->setNetworkMode(false);
    if (m_netClient) {
        m_networkReconnectFailed = false;
        m_netClient->closeConnection();
        delete m_netClient;
        m_netClient = nullptr;
    }
    if (m_glWidget) {
        m_glWidget->setNetworkClient(nullptr);
        m_glWidget->stopNetworkInputLoop();
        m_glWidget->hideReconnectOverlay();
        m_glWidget->setCursor(Qt::ArrowCursor);
    }
    m_engine->setNetworkClient(nullptr);

    m_menu->hide();
    m_hud->show();
    m_hud->resize(1, 1);
    m_hud->move(0, 0);
    auto modeCfg = getModeConfig(mode);
    m_hud->setMode(modeCfg.name);
    m_leaderboard->move(width() - 190, 60);
    m_hud->showHUD();
    m_glWidget->setShowMinimap(m_hud->minimapVisible());
    m_glWidget->setFocus();
    m_glWidget->setCursor(Qt::BlankCursor);
    if (m_engine) m_engine->startGame(mode);
    m_glWidget->update();
}

void MainWindow::openModeSelect() {
    QTimer::singleShot(0, this, [this]() {
        m_subWindowMgr->resize(size());
        m_subWindowMgr->showWindow(m_modeSelectWindow);
    });
}

void MainWindow::openSettings() {
    QTimer::singleShot(0, this, [this]() {
        m_subWindowMgr->resize(size());
        m_subWindowMgr->showWindow(m_settingsWindow);
    });
}

void MainWindow::openRecords() {
    m_recordsWindow->loadRecords();
    QTimer::singleShot(0, this, [this]() {
        m_subWindowMgr->resize(size());
        m_subWindowMgr->showWindow(m_recordsWindow);
    });
}

void MainWindow::openRank() {
    m_rankWindow->refresh("local");
    QTimer::singleShot(0, this, [this]() {
        m_subWindowMgr->resize(size());
        m_subWindowMgr->showWindow(m_rankWindow);
    });
}

void MainWindow::openAchievements() {
    m_achievementWindow->refresh();
    QTimer::singleShot(0, this, [this]() {
        m_subWindowMgr->resize(size());
        m_subWindowMgr->showWindow(m_achievementWindow);
    });
}

void MainWindow::onGameEnded() {
    if (m_returningToMenu) {
        m_returningToMenu = false;
        return;
    }
    m_leaderboard->hide();
    m_hud->hide();
    m_gameOverScreen->showStats(m_engine->lastRecord(), m_engine->lastEloChange(), m_engine->lastGameWasVictory());
    m_subWindowMgr->resize(size());
    m_subWindowMgr->showWindow(m_gameOverScreen);
}

void MainWindow::playAgain() {
    m_subWindowMgr->hide();
    startGame(m_engine->currentMode());
}

void MainWindow::applyDisplaySettings() {
    auto& cfg = Config::instance();
    int idx = qBound(0, cfg.resolutionIndex, kResolutionCount - 1);
    const auto& res = kResolutions[idx];

    switch (cfg.displayMode) {
    case 0: // 窗口模式 - 锁定为预设分辨率
        setWindowState(windowState() & ~Qt::WindowFullScreen);
        setFixedSize(res.width, res.height);
        break;
    case 1: // 无边框窗口 - 锁定为预设分辨率
        setWindowState(windowState() & ~Qt::WindowFullScreen);
        setFixedSize(res.width, res.height);
        setWindowFlags(Qt::Window | Qt::FramelessWindowHint);
        show();
        break;
    case 2: // 全屏 - 解锁尺寸限制
        setMinimumSize(0, 0);
        setMaximumSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX);
        setWindowFlags(Qt::Window);
        show();
        setWindowState(Qt::WindowFullScreen);
        break;
    }
}

void MainWindow::startNetworkedGame(const QString& host, int port, const QString& name,
                                    const QString& roomName, bool createIfMissing, int capacity) {
    if (!m_engine) return;

    auto& cfg = Config::instance();
    cfg.networkMode = true;
    cfg.serverHost = host;
    cfg.serverPort = port;
    cfg.playerName = name;
    cfg.save("config.json");

    // 清理旧连接
    if (m_netClient) {
        m_netClient->closeConnection();
        delete m_netClient;
        m_netClient = nullptr;
    }
    if (m_engine->networkClient()) {
        m_engine->setNetworkClient(nullptr);
    }

    m_netClient = new NetworkClient(this);
    m_engine->setNetworkClient(m_netClient);
    m_glWidget->setNetworkClient(m_netClient);

    // 连接成功后加入指定房间
    connect(m_netClient, &NetworkClient::connected, this, [this, name, roomName, createIfMissing, capacity]() {
        qDebug() << "[MainWindow] connected, joining room" << roomName;
        m_netClient->joinRoom(roomName, name, "free", createIfMissing, capacity);
    }, Qt::QueuedConnection);

    // welcome：切换到游戏视图 + 启动 30Hz input 上行
    connect(m_netClient, &NetworkClient::welcomeReceived, this, [this](int myId, float w, float h) {
        qDebug() << "[MainWindow] welcome: myId=" << myId << "world" << w << "x" << h;
        m_engine->setNetworkMode(true);
        m_engine->setNetworkMyId(myId);

        m_menu->hide();
        m_subWindowMgr->clearAll();
        m_hud->show();
        m_hud->showHUD();
        m_hud->resize(1, 1);
        m_hud->move(0, 0);
        m_hud->setMode(QStringLiteral("联网对战"));
        m_leaderboard->move(width() - 190, 60);
        m_glWidget->setFocus();
        m_glWidget->setCursor(Qt::BlankCursor);
        m_glWidget->hideReconnectOverlay();
        m_glWidget->startNetworkInputLoop();
    }, Qt::QueuedConnection);

    // snapshot：直接交给 GLWidget 应用到 World
    connect(m_netClient, &NetworkClient::snapshotReceived,
            m_glWidget, &GLWidget::onSnapshotReceived, Qt::QueuedConnection);

    // 死亡：触发 GameOverScreen
    connect(m_netClient, &NetworkClient::deathReceived, this, [this](const QString& msg) {
        qDebug() << "[MainWindow] player died:" << msg;
        if (m_engine) m_engine->endGame();
    }, Qt::QueuedConnection);

    // 断线：显示重连遮罩；彻底失败弹窗回主菜单
    connect(m_netClient, &NetworkClient::disconnected, this, [this](const QString& reason) {
        // 如果已经不在网络模式了（被 showMainMenu/startGame 清理过），忽略
        if (!m_netClient) return;
        qWarning() << "[MainWindow] disconnected:" << reason;
        // 正在重连（attempts>0 且 <=3）→ 显示遮罩；已达上限 → 弹窗回主菜单
        if (reason.contains("重连失败") || reason.contains("exhausted")) {
            m_glWidget->stopNetworkInputLoop();
            m_glWidget->hideReconnectOverlay();
            m_glWidget->setCursor(Qt::ArrowCursor);
            m_networkReconnectFailed = true;
            QMessageBox::warning(this, QStringLiteral("网络连接"), reason);
            onBackToMenu();
        } else {
            m_glWidget->showReconnectOverlay(reason);
        }
    }, Qt::QueuedConnection);

    connect(m_netClient, &NetworkClient::errorOccurred, this, [this](const QString& msg) {
        qWarning() << "[MainWindow] network error:" << msg;
        QMessageBox::warning(this, QStringLiteral("网络错误"), msg);
    }, Qt::QueuedConnection);

    QUrl url(QString("ws://%1:%2/ws").arg(host).arg(port));
    qDebug() << "[MainWindow] connecting to" << url.toString();
    m_netClient->connectTo(url, name, "free");
}
