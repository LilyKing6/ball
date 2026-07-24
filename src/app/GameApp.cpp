#include "GameApp.h"
#include "MainWindow.h"
#include "engine/GameEngine.h"
#include "util/Config.h"
#include "storage/DatabaseManager.h"
#include "record/RecordManager.h"
#include "ranking/SeasonManager.h"
#include "achievement/AchievementManager.h"
#include "audio/AudioManager.h"
#include <QApplication>
#include <QDir>

GameApp::GameApp(int argc, char* argv[]) {
    Config::instance().load("config.json");

    m_app = new QApplication(argc, argv);
    m_app->setApplicationName("球球大作战");
    m_app->setApplicationVersion("1.0.0");

    DatabaseManager::instance().initialize("ballbattle.db");
    RecordManager::instance().ensureProfile("local", "Player");
    SeasonManager::instance().checkSeasonRollover("local");
    AchievementManager::instance().loadUnlocked("local");

    auto& audio = AudioManager::instance();
    audio.initialize(QDir::currentPath() + "/resources/sounds");
    auto& cfg = Config::instance();
    audio.setSfxVolume(cfg.sfxVolume);
    audio.setBgmVolume(cfg.bgmVolume);
    audio.setSfxMuted(cfg.sfxMuted);
    audio.setBgmMuted(cfg.bgmMuted);

    m_engine = new GameEngine(this);
    m_window = new MainWindow();
    m_window->setEngine(m_engine);
}

GameApp::~GameApp() {
    Config::instance().save("config.json");
    delete m_window;
    delete m_app;
}

int GameApp::run() {
    m_window->show();

    // 全屏模式需要在 show() 之后设置
    if (Config::instance().displayMode == 2) {
        m_window->setWindowState(Qt::WindowFullScreen);
    }

    return m_app->exec();
}
