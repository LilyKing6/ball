#ifndef CONFIG_H
#define CONFIG_H

#include <QtCore>
#include "input/KeyBinding.h"

struct Config {
    static Config& instance() { static Config c; return c; }

    // World
    float worldWidth = 6000.0f;
    float worldHeight = 6000.0f;

    // Player
    float initialMass = 10.0f;
    float baseSpeed = 300.0f;
    float radiusConstant = 6.0f;
    float splitMinMass = 36.0f;
    float ejectMass = 14.0f;
    int maxCellsPerPlayer = 16;
    float mergeCooldown = 30.0f;
    float splitVelocity = 800.0f;
    float ejectVelocity = 400.0f;
    float splitCooldown = 0.5f;
    float ejectCooldown = 0.1f;

    // Food
    int foodCount = 3000;
    float foodRadius = 3.0f;
    float foodMass = 1.0f;

    // BigBean
    int bigBeanCount = 15;
    float bigBeanMinMass = 100.0f;
    float bigBeanMaxMass = 500.0f;

    // Virus
    int virusCount = 30;
    float virusMass = 100.0f;
    float virusRespawnTime = 10.0f;
    float virusSplitThreshold = 200.0f;
    int virusFragmentCount = 9;
    float virusFragmentVelocity = 500.0f;

    // Spore
    float sporeSpeed = 400.0f;
    float sporeImmunityTime = 0.3f;
    float sporeDecayRate = 0.3f;
    float sporeVelocityDecay = 0.98f;
    int maxSpores = 30;

    // Player limits
    float maxMassPerCell = 40000.0f;

    // AI
    int maxAI = 50;
    float aiPerceptionBase = 800.0f;

    // Physics
    float fixedDt = 1.0f / 120.0f;
    float massRatioForEat = 1.1f;
    float overlapRatioForEat = 0.5f;
    int spatialHashCellSize = 200;

    // Camera
    float cameraSmoothing = 0.1f;
    float baseZoom = 1.0f;
    float zoomMassDivisor = 20.0f;
    float zoomMin = 0.2f;
    float zoomMax = 5.0f;

    // Timing
    int targetFPS = 60;

    // World / respawn
    float respawnDelay = 3.0f;

    // BattleRoyale
    float brZoneDamagePerSec = 50.0f;
    float brShrinkFactor = 0.85f;
    float brShrinkInterval = 30.0f;
    float brMinZoneRadius = 200.0f;

    // Minimap
    int minimapSize = 150;
    int minimapMargin = 10;
    int minimapMaxPlayers = 3;

    // Leaderboard
    int leaderboardMaxEntries = 10;

    // Audio
    float sfxVolume = 1.0f;
    float bgmVolume = 0.5f;
    bool sfxMuted = false;
    bool bgmMuted = false;

    // Display
    int resolutionIndex = 1;   // 0=HD, 1=FHD, 2=2K, 3=4K
    int displayMode = 0;       // 0=Windowed, 1=Borderless, 2=Fullscreen

    // HUD
    int hudQualityPreset = 1;     // 0=低, 1=中, 2=高
    int hudFpsLimit = 0;          // 0=无限制, 30, 60, 120
    float hudScale = 1.0f;        // 0.7 ~ 1.5
    int hudSkillButtonSize = 1;   // 0=小(48), 1=中(64), 2=大(80)
    int hudOverlayMode = 0;       // 0=排行榜, 1=小地图, 2=隐藏

    // Control - 虚拟游标 / 摇杆系统
    int controlMode = 0;          // 0=鼠标游标, 1=虚拟摇杆, 2=双模式(Hybrid)
    int joystickPosition = 0;     // 0=左下, 1=右下, 2=中下, 3=自定义
    float joystickCustomX = 0.15f; // 自定义位置（屏幕比例 0-1，仅 position=3）
    float joystickCustomY = 0.75f;
    bool joystickFixed = true;    // true=固定位置, false=浮动
    float joystickRadius = 500.0f;     // 摇杆半径（世界单位，控制游标最大偏移）
    float joystickDeadzone = 10.0f;    // 防误触阈值（屏幕像素）
    int controlSwitchKey = 0x01000020; // Qt::Key_Shift 默认

    // External Agent Server（WebSocket，用于 RL agent / 网络对战接入）
    bool enableAgentServer = false;       // 默认关闭，需在设置中开启
    int agentServerPort = 8765;           // 监听端口
    int agentBroadcastEvery = 6;          // 每 N 个 GameEngine::update 调用广播一次快照（6 = 10Hz @ 60fps）
    float agentObserveRadius = 1500.0f;   // 视野裁剪半径（本地玩家为中心），单位：世界坐标

    // Network（连接外部 Go 服务端）
    bool networkMode = false;             // 启动游戏时是否走网络模式
    QString serverHost = "127.0.0.1";
    int serverPort = 8765;
    QString playerName = "Player";

    // Key bindings
    KeyBinding keyBindings;

    void load(const QString& path);
    void save(const QString& path) const;
};

#endif // CONFIG_H
