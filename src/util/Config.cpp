#include "Config.h"
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>

#define CFG_LOAD(field, type, method) field = o.value(#field).method(field)
#define CFG_SAVE(field) o[#field] = field

void Config::load(const QString& path) {
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) return;
    QJsonObject o = QJsonDocument::fromJson(f.readAll()).object();

    // World
    CFG_LOAD(worldWidth, double, toDouble);
    CFG_LOAD(worldHeight, double, toDouble);

    // Player
    CFG_LOAD(initialMass, double, toDouble);
    CFG_LOAD(baseSpeed, double, toDouble);
    CFG_LOAD(radiusConstant, double, toDouble);
    CFG_LOAD(splitMinMass, double, toDouble);
    CFG_LOAD(ejectMass, double, toDouble);
    CFG_LOAD(maxCellsPerPlayer, int, toInt);
    CFG_LOAD(mergeCooldown, double, toDouble);
    CFG_LOAD(splitVelocity, double, toDouble);
    CFG_LOAD(ejectVelocity, double, toDouble);
    CFG_LOAD(splitCooldown, double, toDouble);
    CFG_LOAD(ejectCooldown, double, toDouble);

    // Food
    CFG_LOAD(foodCount, int, toInt);
    CFG_LOAD(foodRadius, double, toDouble);
    CFG_LOAD(foodMass, double, toDouble);

    // BigBean
    CFG_LOAD(bigBeanCount, int, toInt);
    CFG_LOAD(bigBeanMinMass, double, toDouble);
    CFG_LOAD(bigBeanMaxMass, double, toDouble);

    // Virus
    CFG_LOAD(virusCount, int, toInt);
    CFG_LOAD(virusMass, double, toDouble);
    CFG_LOAD(virusRespawnTime, double, toDouble);
    CFG_LOAD(virusSplitThreshold, double, toDouble);
    CFG_LOAD(virusFragmentCount, int, toInt);
    CFG_LOAD(virusFragmentVelocity, double, toDouble);

    // Spore
    CFG_LOAD(sporeSpeed, double, toDouble);
    CFG_LOAD(sporeImmunityTime, double, toDouble);
    CFG_LOAD(sporeDecayRate, double, toDouble);
    CFG_LOAD(sporeVelocityDecay, double, toDouble);
    CFG_LOAD(maxSpores, int, toInt);

    // Player limits
    CFG_LOAD(maxMassPerCell, double, toDouble);

    // AI
    CFG_LOAD(maxAI, int, toInt);
    CFG_LOAD(aiPerceptionBase, double, toDouble);

    // Physics
    CFG_LOAD(fixedDt, double, toDouble);
    CFG_LOAD(massRatioForEat, double, toDouble);
    CFG_LOAD(overlapRatioForEat, double, toDouble);
    CFG_LOAD(spatialHashCellSize, int, toInt);

    // Camera
    CFG_LOAD(cameraSmoothing, double, toDouble);
    CFG_LOAD(baseZoom, double, toDouble);
    CFG_LOAD(zoomMassDivisor, double, toDouble);
    CFG_LOAD(zoomMin, double, toDouble);
    CFG_LOAD(zoomMax, double, toDouble);

    // Timing
    CFG_LOAD(targetFPS, int, toInt);

    // World / respawn
    CFG_LOAD(respawnDelay, double, toDouble);

    // BattleRoyale
    CFG_LOAD(brZoneDamagePerSec, double, toDouble);
    CFG_LOAD(brShrinkFactor, double, toDouble);
    CFG_LOAD(brShrinkInterval, double, toDouble);
    CFG_LOAD(brMinZoneRadius, double, toDouble);

    // Minimap
    CFG_LOAD(minimapSize, int, toInt);
    CFG_LOAD(minimapMargin, int, toInt);
    CFG_LOAD(minimapMaxPlayers, int, toInt);

    // Leaderboard
    CFG_LOAD(leaderboardMaxEntries, int, toInt);

    // Audio
    CFG_LOAD(sfxVolume, double, toDouble);
    CFG_LOAD(bgmVolume, double, toDouble);
    CFG_LOAD(sfxMuted, bool, toBool);
    CFG_LOAD(bgmMuted, bool, toBool);

    // Display
    CFG_LOAD(resolutionIndex, int, toInt);
    CFG_LOAD(displayMode, int, toInt);

    // HUD
    CFG_LOAD(hudQualityPreset, int, toInt);
    CFG_LOAD(hudFpsLimit, int, toInt);
    CFG_LOAD(hudScale, double, toDouble);
    CFG_LOAD(hudSkillButtonSize, int, toInt);
    CFG_LOAD(hudOverlayMode, int, toInt);

    // Control
    CFG_LOAD(controlMode, int, toInt);
    CFG_LOAD(joystickPosition, int, toInt);
    CFG_LOAD(joystickCustomX, double, toDouble);
    CFG_LOAD(joystickCustomY, double, toDouble);
    CFG_LOAD(joystickFixed, bool, toBool);
    CFG_LOAD(joystickRadius, double, toDouble);
    CFG_LOAD(joystickDeadzone, double, toDouble);
    CFG_LOAD(controlSwitchKey, int, toInt);

    // Agent server
    CFG_LOAD(enableAgentServer, bool, toBool);
    CFG_LOAD(agentServerPort, int, toInt);
    CFG_LOAD(agentBroadcastEvery, int, toInt);
    CFG_LOAD(agentObserveRadius, double, toDouble);

    // Network (Qt 客户端连 Go 服务端)
    CFG_LOAD(networkMode, bool, toBool);
    networkMode = false; // 启动时强制重置，避免上次保存的 true 导致单机进入无服务端状态
    {
        QString s = o.value("serverHost").toString();
        if (!s.isEmpty()) serverHost = s;
    }
    CFG_LOAD(serverPort, int, toInt);
    {
        QString s = o.value("playerName").toString();
        if (!s.isEmpty()) playerName = s;
    }
}

void Config::save(const QString& path) const {
    QJsonObject o;

    // World
    CFG_SAVE(worldWidth);
    CFG_SAVE(worldHeight);

    // Player
    CFG_SAVE(initialMass);
    CFG_SAVE(baseSpeed);
    CFG_SAVE(radiusConstant);
    CFG_SAVE(splitMinMass);
    CFG_SAVE(ejectMass);
    CFG_SAVE(maxCellsPerPlayer);
    CFG_SAVE(mergeCooldown);
    CFG_SAVE(splitVelocity);
    CFG_SAVE(ejectVelocity);
    CFG_SAVE(splitCooldown);
    CFG_SAVE(ejectCooldown);

    // Food
    CFG_SAVE(foodCount);
    CFG_SAVE(foodRadius);
    CFG_SAVE(foodMass);

    // BigBean
    CFG_SAVE(bigBeanCount);
    CFG_SAVE(bigBeanMinMass);
    CFG_SAVE(bigBeanMaxMass);

    // Virus
    CFG_SAVE(virusCount);
    CFG_SAVE(virusMass);
    CFG_SAVE(virusRespawnTime);
    CFG_SAVE(virusSplitThreshold);
    CFG_SAVE(virusFragmentCount);
    CFG_SAVE(virusFragmentVelocity);

    // Spore
    CFG_SAVE(sporeSpeed);
    CFG_SAVE(sporeImmunityTime);
    CFG_SAVE(sporeDecayRate);
    CFG_SAVE(sporeVelocityDecay);
    CFG_SAVE(maxSpores);

    // Player limits
    CFG_SAVE(maxMassPerCell);

    // AI
    CFG_SAVE(maxAI);
    CFG_SAVE(aiPerceptionBase);

    // Physics
    CFG_SAVE(fixedDt);
    CFG_SAVE(massRatioForEat);
    CFG_SAVE(overlapRatioForEat);
    CFG_SAVE(spatialHashCellSize);

    // Camera
    CFG_SAVE(cameraSmoothing);
    CFG_SAVE(baseZoom);
    CFG_SAVE(zoomMassDivisor);
    CFG_SAVE(zoomMin);
    CFG_SAVE(zoomMax);

    // Timing
    CFG_SAVE(targetFPS);

    // World / respawn
    CFG_SAVE(respawnDelay);

    // BattleRoyale
    CFG_SAVE(brZoneDamagePerSec);
    CFG_SAVE(brShrinkFactor);
    CFG_SAVE(brShrinkInterval);
    CFG_SAVE(brMinZoneRadius);

    // Minimap
    CFG_SAVE(minimapSize);
    CFG_SAVE(minimapMargin);
    CFG_SAVE(minimapMaxPlayers);

    // Leaderboard
    CFG_SAVE(leaderboardMaxEntries);

    // Audio
    CFG_SAVE(sfxVolume);
    CFG_SAVE(bgmVolume);
    CFG_SAVE(sfxMuted);
    CFG_SAVE(bgmMuted);

    // Display
    CFG_SAVE(resolutionIndex);
    CFG_SAVE(displayMode);

    // HUD
    CFG_SAVE(hudQualityPreset);
    CFG_SAVE(hudFpsLimit);
    CFG_SAVE(hudScale);
    CFG_SAVE(hudSkillButtonSize);
    CFG_SAVE(hudOverlayMode);

    // Control
    CFG_SAVE(controlMode);
    CFG_SAVE(joystickPosition);
    CFG_SAVE(joystickCustomX);
    CFG_SAVE(joystickCustomY);
    CFG_SAVE(joystickFixed);
    CFG_SAVE(joystickRadius);
    CFG_SAVE(joystickDeadzone);
    CFG_SAVE(controlSwitchKey);

    // Agent server
    CFG_SAVE(enableAgentServer);
    CFG_SAVE(agentServerPort);
    CFG_SAVE(agentBroadcastEvery);
    CFG_SAVE(agentObserveRadius);

    // Network
    o["serverHost"] = serverHost;
    CFG_SAVE(serverPort);
    o["playerName"] = playerName;
    // networkMode 故意不持久化为 true，避免 launch 时无服务端

    QFile f(path);
    if (f.open(QIODevice::WriteOnly))
        f.write(QJsonDocument(o).toJson());
}
