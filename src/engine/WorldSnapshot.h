#ifndef WORLDSNAPSHOT_H
#define WORLDSNAPSHOT_H

#include <QJsonObject>
#include <QJsonArray>
#include <QVector>
#include <QString>

// 观察层结构：用于把 World 状态序列化为 JSON，发送给外部 Agent（RL/网络对战）
// 故意与游戏内部 Entity 分离：外部 agent 只看必要信息，无内部状态字段

struct CellObservation {
    float x = 0;
    float y = 0;
    float mass = 0;

    QJsonObject toJson() const {
        return QJsonObject{
            {"x", x}, {"y", y}, {"mass", mass}
        };
    }
    static CellObservation fromJson(const QJsonObject& o) {
        CellObservation c;
        c.x = static_cast<float>(o.value("x").toDouble());
        c.y = static_cast<float>(o.value("y").toDouble());
        c.mass = static_cast<float>(o.value("mass").toDouble());
        return c;
    }
};

struct PlayerObservation {
    int id = 0;
    QString name;
    int team = 0;
    int shieldCount = 0;
    bool isLocal = false;
    bool isAlive = false;
    QVector<CellObservation> cells;

    QJsonObject toJson() const {
        QJsonArray cellsArr;
        for (const auto& c : cells) cellsArr.append(c.toJson());
        return QJsonObject{
            {"id", id},
            {"name", name},
            {"team", team},
            {"shield", shieldCount},
            {"isLocal", isLocal},
            {"isAlive", isAlive},
            {"cells", cellsArr}
        };
    }
    static PlayerObservation fromJson(const QJsonObject& o) {
        PlayerObservation p;
        p.id = o.value("id").toInt();
        p.name = o.value("name").toString();
        p.team = o.value("team").toInt();
        p.shieldCount = o.value("shield").toInt();
        p.isLocal = o.value("isLocal").toBool();
        p.isAlive = o.value("isAlive").toBool();
        const auto arr = o.value("cells").toArray();
        for (const auto& v : arr) p.cells.append(CellObservation::fromJson(v.toObject()));
        return p;
    }
};

struct FoodObservation {
    float x = 0;
    float y = 0;

    QJsonObject toJson() const { return QJsonObject{{"x", x}, {"y", y}}; }
    static FoodObservation fromJson(const QJsonObject& o) {
        FoodObservation f;
        f.x = static_cast<float>(o.value("x").toDouble());
        f.y = static_cast<float>(o.value("y").toDouble());
        return f;
    }
};

struct VirusObservation {
    float x = 0;
    float y = 0;

    QJsonObject toJson() const { return QJsonObject{{"x", x}, {"y", y}}; }
    static VirusObservation fromJson(const QJsonObject& o) {
        VirusObservation v;
        v.x = static_cast<float>(o.value("x").toDouble());
        v.y = static_cast<float>(o.value("y").toDouble());
        return v;
    }
};

struct SporeObservation {
    float x = 0;
    float y = 0;
    float mass = 0;
    float radius = 0;

    QJsonObject toJson() const { return QJsonObject{{"x", x}, {"y", y}, {"mass", mass}, {"radius", radius}}; }
    static SporeObservation fromJson(const QJsonObject& o) {
        SporeObservation s;
        s.x = static_cast<float>(o.value("x").toDouble());
        s.y = static_cast<float>(o.value("y").toDouble());
        s.mass = static_cast<float>(o.value("mass").toDouble());
        s.radius = static_cast<float>(o.value("radius").toDouble());
        return s;
    }
};

struct BigBeanObservation {
    float x = 0;
    float y = 0;
    float mass = 0;

    QJsonObject toJson() const { return QJsonObject{{"x", x}, {"y", y}, {"mass", mass}}; }
    static BigBeanObservation fromJson(const QJsonObject& o) {
        BigBeanObservation b;
        b.x = static_cast<float>(o.value("x").toDouble());
        b.y = static_cast<float>(o.value("y").toDouble());
        b.mass = static_cast<float>(o.value("mass").toDouble());
        return b;
    }
};

// 游戏世界快照：包含外部 Agent 需要观察的全部信息
struct WorldSnapshot {
    int tickId = 0;             // 帧序号（单调递增）
    float gameTime = 0;         // 游戏已进行秒数
    float worldWidth = 0;
    float worldHeight = 0;
    int gameMode = 0;           // 0=Free, 1=SpeedFree, 2=Team, 3=BattleRoyale

    // 大逃杀专属
    float safeZoneRadius = 0;
    float safeZoneCenterX = 0;
    float safeZoneCenterY = 0;
    int shrinkPhase = 0;
    float timeToNextShrink = -1;

    QVector<PlayerObservation> players;
    QVector<FoodObservation> foods;
    QVector<VirusObservation> viruses;
    QVector<SporeObservation> spores;
    QVector<BigBeanObservation> bigBeans;

    QJsonObject toJson() const {
        QJsonArray playersArr;
        for (const auto& p : players) playersArr.append(p.toJson());
        QJsonArray foodsArr;
        for (const auto& f : foods) foodsArr.append(f.toJson());
        QJsonArray virusesArr;
        for (const auto& v : viruses) virusesArr.append(v.toJson());
        QJsonArray sporesArr;
        for (const auto& s : spores) sporesArr.append(s.toJson());
        QJsonArray bigBeansArr;
        for (const auto& b : bigBeans) bigBeansArr.append(b.toJson());

        return QJsonObject{
            {"tickId", tickId},
            {"gameTime", gameTime},
            {"worldWidth", worldWidth},
            {"worldHeight", worldHeight},
            {"gameMode", gameMode},
            {"safeZoneRadius", safeZoneRadius},
            {"safeZoneCenterX", safeZoneCenterX},
            {"safeZoneCenterY", safeZoneCenterY},
            {"shrinkPhase", shrinkPhase},
            {"timeToNextShrink", timeToNextShrink},
            {"players", playersArr},
            {"foods", foodsArr},
            {"viruses", virusesArr},
            {"spores", sporesArr},
            {"bigBeans", bigBeansArr}
        };
    }

    static WorldSnapshot fromJson(const QJsonObject& o) {
        WorldSnapshot s;
        s.tickId = o.value("tickId").toInt();
        s.gameTime = static_cast<float>(o.value("gameTime").toDouble());
        s.worldWidth = static_cast<float>(o.value("worldWidth").toDouble());
        s.worldHeight = static_cast<float>(o.value("worldHeight").toDouble());
        s.gameMode = o.value("gameMode").toInt();
        s.safeZoneRadius = static_cast<float>(o.value("safeZoneRadius").toDouble());
        s.safeZoneCenterX = static_cast<float>(o.value("safeZoneCenterX").toDouble());
        s.safeZoneCenterY = static_cast<float>(o.value("safeZoneCenterY").toDouble());
        s.shrinkPhase = o.value("shrinkPhase").toInt();
        s.timeToNextShrink = static_cast<float>(o.value("timeToNextShrink").toDouble());
        for (const auto& v : o.value("players").toArray()) s.players.append(PlayerObservation::fromJson(v.toObject()));
        for (const auto& v : o.value("foods").toArray()) s.foods.append(FoodObservation::fromJson(v.toObject()));
        for (const auto& v : o.value("viruses").toArray()) s.viruses.append(VirusObservation::fromJson(v.toObject()));
        for (const auto& v : o.value("spores").toArray()) s.spores.append(SporeObservation::fromJson(v.toObject()));
        for (const auto& v : o.value("bigBeans").toArray()) s.bigBeans.append(BigBeanObservation::fromJson(v.toObject()));
        return s;
    }

    // 从 World 构建一帧快照
    //   tickId: 帧序号
    //   observerCenter/observerRadius: 视野裁剪中心/半径（世界坐标）；radius<=0 = 不裁剪
    static WorldSnapshot fromWorld(const class World& world, int tickId,
                                    float centerX = 0, float centerY = 0, float observerRadius = -1);
};

#endif // WORLDSNAPSHOT_H
