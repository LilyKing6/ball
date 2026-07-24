#ifndef GAMEENGINE_H
#define GAMEENGINE_H

#include <QObject>
#include <QElapsedTimer>
#include "GameState.h"
#include "World.h"
#include "record/GameRecord.h"
#include "GameMode.h"
#include "IController.h"

class ExternalAgentServer;
class NetworkClient;
class WorldSnapshot;

class GameEngine : public QObject {
    Q_OBJECT
public:
    explicit GameEngine(QObject* parent = nullptr);
    ~GameEngine() override;

    GameState& state() { return m_state; }
    World& world() { return m_world; }
    const World& world() const { return m_world; }

    void startGame(GameMode mode = GameMode::FreeMode);
    void startNetworkedGame();
    void pauseGame();
    void resumeGame();
    void endGame();

    void update(float dt);
    void setLocalPlayerMousePos(const Vec2& worldPos);
    void setLocalPlayerCursor(const Vec2& worldPos);  // 设置虚拟游标位置
    void splitLocalPlayer();
    void ejectFromLocalPlayer();

    // === 网络模式 ===
    void setNetworkClient(NetworkClient* client);
    NetworkClient* networkClient() const { return m_networkClient; }
    void setNetworkMode(bool on) { m_networkMode = on; }
    bool networkMode() const { return m_networkMode; }
    void applyNetworkSnapshot(const WorldSnapshot& snap);
    // 返回累积的玩家输入（每帧由 GLWidget 取走发送给服务端）
    PlayerInput& pendingInput() { return m_pendingInput; }
    void clearPendingInputFlags();
    int networkMyId() const { return m_networkMyId; }
    void setNetworkMyId(int id) { m_networkMyId = id; }

    void addKill(const QString& victimName, float victimMass);
    void addFoodEaten();
    void addSplit();
    void addEject();

    const GameRecord& lastRecord() const { return m_lastRecord; }
    int lastEloChange() const { return m_lastEloChange; }
    GameMode currentMode() const { return m_gameMode; }
    bool lastGameWasVictory() const { return m_lastGameVictory; }

    // Debug methods
    void setLocalPlayerMass(float mass);
    void setGodMode(bool enabled);
    void setSpeedBoost(bool enabled);
    void spawnFoodNearPlayer(int count);
    void spawnBigBeanNearPlayer();
    void teleportLocalPlayerToCenter();

signals:
    void gameStarted();
    void gameEnded();

private:
    GameState m_state;
    World m_world;
    float m_accumulator = 0.0f;
    QElapsedTimer m_gameTimer;

    int m_foodEaten = 0;
    int m_splitCount = 0;
    int m_ejectCount = 0;
    int m_killCount = 0;
    float m_maxMass = 0;
    QVector<KillEvent> m_killTimeline;
    GameRecord m_lastRecord;
    int m_lastEloChange = 0;
    bool m_lastGameVictory = false;
    GameMode m_gameMode = GameMode::FreeMode;

    // Debug flags
    bool m_godMode = false;
    bool m_speedBoost = false;

    // External agent server（仅在 Config.enableAgentServer 为 true 时启动）
    ExternalAgentServer* m_agentServer = nullptr;
    int m_tickId = 0;
    int m_broadcastCounter = 0;

    // 网络模式
    bool m_networkMode = false;
    NetworkClient* m_networkClient = nullptr;
    PlayerInput m_pendingInput;
    int m_networkMyId = 0;

    void initAgentServer();
    void shutdownAgentServer();
    void broadcastSnapshotIfDue();

    void saveRecord();
};

#endif // GAMEENGINE_H
