#ifndef KEYBINDING_H
#define KEYBINDING_H

#include <QMap>
#include <QString>
#include <QJsonObject>

// 游戏内可配置按键的操作
enum class GameAction {
    Split,           // 分裂
    Eject,           // 吐球
    ToggleDebug,     // 调试面板
    ToggleControlMode, // Hybrid 模式切换游标/摇杆
};

// 键位绑定：操作 -> Qt::Key
struct KeyBinding {
    QMap<GameAction, int> bindings;

    KeyBinding();

    int key(GameAction action) const;
    void setKey(GameAction action, int qtKey);
    QString actionName(GameAction action) const;
    QString keyName(GameAction action) const;

    // 序列化为 JSON 对象
    QJsonObject toJson() const;
    static KeyBinding fromJson(const QJsonObject& o);
};

#endif // KEYBINDING_H
