#ifndef ICONTROLLER_H
#define ICONTROLLER_H

#include "util/Math.h"
#include <QString>
#include <QJsonObject>

class Player;
class World;

// 玩家一帧输入：所有控制器（人类/AI/外部 agent）的统一输出
struct PlayerInput {
    Vec2 mouseWorldPos = {0, 0};    // 兼容旧字段
    Vec2 virtualCursor = {0, 0};    // 虚拟游标（世界坐标），用于移动和吐球
    bool wantSplit = false;          // 本帧请求分裂
    bool wantEject = false;          // 本帧请求吐球

    QJsonObject toJson() const;
    static PlayerInput fromJson(const QJsonObject& o);
};

// 控制器抽象接口：Human / AI / External(网络) 都实现此接口
class IController {
public:
    virtual ~IController() = default;

    // 每帧由 World/GameEngine 调用，控制器返回玩家本帧的输入
    virtual PlayerInput sample(const Player& self, const World& world, float dt) = 0;

    // 控制器类型标识，用于序列化/调试
    virtual QString type() const = 0;
};

#endif // ICONTROLLER_H
