#include "GameEngine.h"
#include "util/Config.h"
#include "util/Random.h"
#include "entity/BigBean.h"
#include "record/RecordManager.h"
#include "ranking/RankSystem.h"
#include "achievement/AchievementManager.h"
#include "storage/DatabaseManager.h"
#include "audio/AudioManager.h"
#include "engine/WorldSnapshot.h"
#include "engine/IController.h"
#include "network/ExternalAgentServer.h"
#include "network/NetworkClient.h"
#include <QDebug>
#include <QSqlQuery>
#include <algorithm>

GameEngine::GameEngine(QObject* parent) : QObject(parent) {
    initAgentServer();
}

GameEngine::~GameEngine() {
    shutdownAgentServer();
}

void GameEngine::initAgentServer() {
    auto& cfg = Config::instance();
    if (!cfg.enableAgentServer) return;
    if (m_agentServer) return;

    m_agentServer = new ExternalAgentServer(static_cast<quint16>(cfg.agentServerPort), this);
    connect(m_agentServer, &ExternalAgentServer::inputReceived,
            this, [this](int playerId, const QJsonObject& inputJson) {
        // 解析为 PlayerInput 并应用到本地玩家（默认 playerId=0/1 = local）
        // 暂时只把 cursor / split / eject 应用到本地玩家
        if (!m_world.localPlayer()) return;
        PlayerInput in = PlayerInput::fromJson(inputJson);
        m_world.localPlayer()->virtualCursor = in.virtualCursor;
        m_world.localPlayer()->mouseWorldPos = in.virtualCursor;
        if (in.wantSplit) splitLocalPlayer();
        if (in.wantEject) ejectFromLocalPlayer();
    });
}

void GameEngine::shutdownAgentServer() {
    if (m_agentServer) {
        m_agentServer->deleteLater();
        m_agentServer = nullptr;
    }
}

void GameEngine::broadcastSnapshotIfDue() {
    if (!m_agentServer) return;
    auto& cfg = Config::instance();
    int every = qMax(1, cfg.agentBroadcastEvery);
    m_broadcastCounter++;
    if (m_broadcastCounter < every) return;
    m_broadcastCounter = 0;

    // 视野中心 = 本地玩家 com（若有）
    Vec2 center{m_world.width() / 2, m_world.height() / 2};
    if (m_world.localPlayer()) center = m_world.localPlayer()->centerOfMass();
    float radius = cfg.agentObserveRadius > 0 ? cfg.agentObserveRadius : -1;

    WorldSnapshot snap = WorldSnapshot::fromWorld(m_world, m_tickId, center.x, center.y, radius);
    m_agentServer->broadcastSnapshot(snap.toJson());
}

void GameEngine::startGame(GameMode mode) {
    m_gameMode = mode;
    m_state.set(GameStateEnum::Playing);
    m_accumulator = 0.0f;
    m_world.init(mode);

    m_gameTimer.start();
    m_foodEaten = 0;
    m_splitCount = 0;
    m_ejectCount = 0;
    m_killCount = 0;
    m_maxMass = 0;
    m_killTimeline.clear();

    AudioManager::instance().playSfx("game_start");
    AudioManager::instance().playBgm("bgm");

    qDebug() << "Game started";
    emit gameStarted();
}

void GameEngine::startNetworkedGame() {
    // 网络模式：本地不跑物理，状态由 snapshot 覆盖
    m_gameMode = GameMode::FreeMode;
    m_state.set(GameStateEnum::Playing);
    m_accumulator = 0.0f;
    // 不调用 m_world.init() —— World 会在收到第一个 snapshot 时 applySnapshot 重建
    m_gameTimer.start();
    m_foodEaten = 0;
    m_splitCount = 0;
    m_ejectCount = 0;
    m_killCount = 0;
    m_maxMass = 0;
    m_killTimeline.clear();
    setNetworkMode(true);
    qDebug() << "Networked game started";
    emit gameStarted();
}

void GameEngine::pauseGame() {
    if (m_state.current() == GameStateEnum::Playing)
        m_state.set(GameStateEnum::Paused);
}

void GameEngine::resumeGame() {
    if (m_state.current() == GameStateEnum::Paused)
        m_state.set(GameStateEnum::Playing);
}

void GameEngine::endGame() {
    if (m_state.current() == GameStateEnum::GameOver) return; // 防止重复触发
    m_state.set(GameStateEnum::GameOver);

    // 网络模式：跳过本地排名/ELO 计算，直接发信号
    if (m_networkMode) {
        m_lastGameVictory = false;
        AudioManager::instance().playSfx("game_over");
        emit gameEnded();
        return;
    }

    auto modeCfg = getModeConfig(m_gameMode);

    // Determine victory
    m_lastGameVictory = m_world.localPlayerWon() || m_world.teamAWon();

    // 时限到达：根据模式判断胜利条件
    if (!m_lastGameVictory && m_world.localPlayer()) {
        float elapsed = m_gameTimer.elapsed() / 1000.0f;
        bool timeUp = modeCfg.timeLimitSeconds > 0 && elapsed >= modeCfg.timeLimitSeconds;

        if (timeUp) {
            if (m_gameMode == GameMode::TeamMode) {
                // 团战：本地玩家所在队伍质量高于对方即获胜
                float teamA = m_world.teamAMass();
                float teamB = m_world.teamBMass();
                int myTeam = m_world.localPlayer()->team;
                if (myTeam == 1 && teamA > teamB) m_lastGameVictory = true;
                else if (myTeam == 2 && teamB > teamA) m_lastGameVictory = true;
            } else {
                // 其他模式：质量排名第一即获胜
                float localMass = m_world.localPlayer()->totalMass();
                bool isFirst = true;
                for (const auto& p : m_world.players()) {
                    if (p.totalMass() > localMass) { isFirst = false; break; }
                }
                if (isFirst) m_lastGameVictory = true;
            }
        }
    }

    AudioManager::instance().playSfx("game_over");
    AudioManager::instance().stopBgm();
    saveRecord();
    emit gameEnded();
}

void GameEngine::update(float dt) {
    if (m_state.current() != GameStateEnum::Playing) return;

    // 网络模式：服务端权威，本地不跑物理。snapshot 由外部应用到 World
    if (m_networkMode) return;

    auto& cfg = Config::instance();
    m_accumulator += dt;

    m_world.resetFrameCounters();

    while (m_accumulator >= cfg.fixedDt) {
        m_world.update(cfg.fixedDt);
        m_accumulator -= cfg.fixedDt;
        m_tickId++;
    }

    // 广播 snapshot 给外部 agent（若已启动）
    broadcastSnapshotIfDue();

    m_foodEaten += m_world.frameFoodEaten();
    for (const auto& fk : m_world.frameKills()) {
        addKill(fk.victimName, fk.victimMass, fk.isSplitKill, fk.isVirusKill);
    }

    // Reset counters on respawn
    if (m_world.localPlayerDied()) {
        m_foodEaten = 0;
        m_splitCount = 0;
        m_ejectCount = 0;
        m_killCount = 0;
        m_splitKillCount = 0;
        m_virusKillCount = 0;
        m_maxMass = 0;
        m_killTimeline.clear();
        m_gameTimer.restart();
    }

    if (m_world.localPlayer()) {
        float mass = m_world.localPlayer()->totalMass();
        if (mass > m_maxMass) m_maxMass = mass;
        AchievementManager::instance().checkMassRealtime("local", mass);
    }

    // Check time limit
    auto modeCfg = getModeConfig(m_gameMode);
    if (modeCfg.timeLimitSeconds > 0) {
        float elapsedSeconds = m_gameTimer.elapsed() / 1000.0f;
        if (elapsedSeconds >= modeCfg.timeLimitSeconds) {
            qDebug() << "Time limit reached:" << elapsedSeconds << "s";
            endGame();
            return;
        }
    }

    // Check win conditions
    if (m_world.localPlayerWon()) {
        qDebug() << "Local player won - ending game";
        endGame();
        return;
    }
    if (m_world.teamAWon() || m_world.teamBWon()) {
        qDebug() << "Team victory - ending game";
        endGame();
        return;
    }
}

void GameEngine::setLocalPlayerMousePos(const Vec2& worldPos) {
    if (m_world.localPlayer()) {
        m_world.localPlayer()->mouseWorldPos = worldPos;
        m_world.localPlayer()->virtualCursor = worldPos;  // 默认游标 = 鼠标
    }
}

void GameEngine::setLocalPlayerCursor(const Vec2& worldPos) {
    if (m_world.localPlayer()) {
        m_world.localPlayer()->virtualCursor = worldPos;
        m_world.localPlayer()->mouseWorldPos = worldPos;
        // 网络模式：存储到 pendingInput
        if (m_networkMode) {
            m_pendingInput.virtualCursor = worldPos;
            m_pendingInput.mouseWorldPos = worldPos;
        }
    }
}

void GameEngine::splitLocalPlayer() {
    if (!m_world.localPlayer()) return;
    // 网络模式：设置标志，由 GLWidget 定期取走发送
    if (m_networkMode) {
        m_pendingInput.wantSplit = true;
        return;
    }
    auto* p = m_world.localPlayer();
    Vec2 dir = (p->virtualCursor - p->centerOfMass()).normalized();
    if (dir.lengthSq() < 0.01f) dir = {1, 0};
    QVector<Cell> newCells;
    p->split(dir, newCells);
    for (auto& c : newCells) p->cells.append(c);
    m_splitCount++;
    AudioManager::instance().playSfx("split");
}

void GameEngine::ejectFromLocalPlayer() {
    if (!m_world.localPlayer()) return;
    // 网络模式：设置标志，由 GLWidget 定期取走发送
    if (m_networkMode) {
        m_pendingInput.wantEject = true;
        return;
    }
    auto* p = m_world.localPlayer();
    p->ejectToward(p->virtualCursor, m_world.spores());
    m_ejectCount++;
    AudioManager::instance().playSfx("eject");
}

void GameEngine::setNetworkClient(NetworkClient* client) {
    m_networkClient = client;
}

void GameEngine::applyNetworkSnapshot(const WorldSnapshot& snap) {
    int myId = m_networkClient ? static_cast<int>(m_networkClient->myPlayerId()) : m_networkMyId;
    m_world.applySnapshot(snap, myId);
}

void GameEngine::clearPendingInputFlags() {
    m_pendingInput.wantSplit = false;
    m_pendingInput.wantEject = false;
}

void GameEngine::addKill(const QString& victimName, float victimMass, bool isSplitKill, bool isVirusKill) {
    m_killCount++;
    if (isSplitKill) m_splitKillCount++;
    if (isVirusKill) m_virusKillCount++;
    m_killTimeline.append({victimName, m_gameTimer.elapsed() / 1000.0f, victimMass, isSplitKill, isVirusKill});
    AudioManager::instance().playSfx("kill");
}

void GameEngine::addFoodEaten() { m_foodEaten++; }
void GameEngine::addSplit() { m_splitCount++; }
void GameEngine::addEject() { m_ejectCount++; }

// Debug methods
void GameEngine::setLocalPlayerMass(float mass) {
    auto* p = m_world.localPlayer();
    if (!p) return;
    int aliveCount = 0;
    for (auto& c : p->cells) if (c.alive) aliveCount++;
    if (aliveCount == 0) return;
    float perCell = mass / aliveCount;
    for (auto& c : p->cells) {
        if (c.alive) c.mass = perCell;
    }
}

void GameEngine::setGodMode(bool enabled) {
    m_godMode = enabled;
}

void GameEngine::setSpeedBoost(bool enabled) {
    m_speedBoost = enabled;
}

void GameEngine::spawnFoodNearPlayer(int count) {
    auto* p = m_world.localPlayer();
    if (!p) return;
    Vec2 center = p->centerOfMass();
    auto& cfg = Config::instance();
    for (int i = 0; i < count; i++) {
        Food f;
        f.pos = {center.x + randFloat(-300, 300), center.y + randFloat(-300, 300)};
        f.mass = cfg.foodMass;
        f.color = QColor::fromHsv(randInt(0, 359), 200, 230);
        f.alive = true;
        m_world.foods().append(f);
    }
}

void GameEngine::spawnBigBeanNearPlayer() {
    auto* p = m_world.localPlayer();
    if (!p) return;
    Vec2 center = p->centerOfMass();
    auto& cfg = Config::instance();
    BigBean bb;
    bb.pos = {center.x + randFloat(-200, 200), center.y + randFloat(-200, 200)};
    bb.mass = randFloat(cfg.bigBeanMinMass, cfg.bigBeanMaxMass);
    bb.color = QColor::fromHsv(randInt(0, 359), 220, 255);
    m_world.bigBeans().append(bb);
}

void GameEngine::teleportLocalPlayerToCenter() {
    auto* p = m_world.localPlayer();
    if (!p) return;
    float cx = m_world.width() / 2;
    float cy = m_world.height() / 2;
    for (auto& c : p->cells) {
        if (c.alive) c.pos = {cx, cy};
    }
}

void GameEngine::saveRecord() {
    GameRecord rec;
    rec.duration = m_gameTimer.elapsed() / 1000.0f;
    rec.maxMass = m_maxMass;
    rec.killCount = m_killCount;
    rec.foodEaten = m_foodEaten;
    rec.splitCount = m_splitCount;
    rec.ejectCount = m_ejectCount;
    rec.splitKillCount = m_splitKillCount;
    rec.virusKillCount = m_virusKillCount;
    rec.killTimeline = m_killTimeline;
    rec.totalPlayers = m_world.players().size();
    rec.mode = "single";

    // Set mode string for record
    switch (m_gameMode) {
    case GameMode::FreeMode:     rec.mode = "free"; break;
    case GameMode::SpeedFree:    rec.mode = "speed"; break;
    case GameMode::TeamMode:     rec.mode = "team"; break;
    case GameMode::BattleRoyale: rec.mode = "battleroyale"; break;
    }

    if (m_world.localPlayer()) {
        rec.finalMass = m_world.localPlayer()->totalMass();
    }

    // Calculate rank in match
    QVector<float> masses;
    for (auto& p : m_world.players()) {
        float m = p.totalMass();
        if (m > 0) masses.append(m);
    }
    std::sort(masses.begin(), masses.end(), std::greater<float>());
    rec.rankInMatch = 1;
    float localMass = rec.finalMass;
    for (int i = 0; i < masses.size(); i++) {
        if (qFuzzyCompare(masses[i], localMass) || masses[i] <= localMass) {
            rec.rankInMatch = i + 1;
            break;
        }
    }

    // Determine death cause
    if (!m_world.localPlayer() || m_world.localPlayer()->totalMass() <= 0) {
        rec.deathCause = "eaten";
    }

    RecordManager::instance().saveRecord("local", rec);
    RecordManager::instance().updateStats("local", rec);

    // Calculate ELO change based on kills
    auto& db = DatabaseManager::instance().database();
    QSqlQuery q(db);
    q.prepare("SELECT elo FROM player_profile WHERE player_id = ?");
    q.addBindValue("local");
    int localElo = 1000;
    if (q.exec() && q.next()) localElo = q.value("elo").toInt();

    int totalEloChange = 0;
    float localMassForElo = rec.finalMass;

    for (const auto& kill : m_killTimeline) {
        int victimElo = 1000;
        QSqlQuery vq(db);
        vq.prepare("SELECT elo FROM player_profile WHERE name = ?");
        vq.addBindValue(kill.victimName);
        if (vq.exec() && vq.next()) victimElo = vq.value("elo").toInt();

        int change = RankSystem::calculateEloChange(localElo, victimElo, localMassForElo, kill.victimMass);
        totalEloChange += change;
    }

    int newElo = localElo + totalEloChange;
    QString tierName = RankSystem::tierInfoForElo(newElo).name;

    QSqlQuery uq(db);
    uq.prepare("UPDATE player_profile SET elo = ?, rank_tier = ? WHERE player_id = ?");
    uq.addBindValue(newElo);
    uq.addBindValue(tierName);
    uq.addBindValue("local");
    uq.exec();

    rec.eloChange = totalEloChange;
    m_lastRecord = rec;
    m_lastEloChange = totalEloChange;
    qDebug() << "Record saved. Duration:" << rec.duration << "s, MaxMass:" << rec.maxMass
             << "ELO:" << localElo << "->" << newElo << "(" << (totalEloChange >= 0 ? "+" : "") << totalEloChange << ")";

    // Check achievements
    auto& ach = AchievementManager::instance();
    ach.checkSingleGame("local", rec);
    ach.checkEloTier("local", newElo, tierName);
}
