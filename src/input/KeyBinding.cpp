#include "KeyBinding.h"
#include <QKeySequence>

KeyBinding::KeyBinding() {
    // 默认键位
    bindings[GameAction::Split] = Qt::Key_Space;
    bindings[GameAction::Eject] = Qt::Key_E;
    bindings[GameAction::ToggleDebug] = Qt::Key_F3;
    bindings[GameAction::ToggleControlMode] = Qt::Key_Shift;
}

int KeyBinding::key(GameAction action) const {
    return bindings.value(action, 0);
}

void KeyBinding::setKey(GameAction action, int qtKey) {
    bindings[action] = qtKey;
}

QString KeyBinding::actionName(GameAction action) const {
    switch (action) {
    case GameAction::Split: return QStringLiteral("分裂");
    case GameAction::Eject: return QStringLiteral("吐球");
    case GameAction::ToggleDebug: return QStringLiteral("调试面板");
    case GameAction::ToggleControlMode: return QStringLiteral("切换控制模式");
    }
    return QStringLiteral("未知");
}

QString KeyBinding::keyName(GameAction action) const {
    int k = key(action);
    if (k == 0) return QStringLiteral("未绑定");
    return QKeySequence(k).toString(QKeySequence::NativeText);
}

QJsonObject KeyBinding::toJson() const {
    QJsonObject o;
    for (auto it = bindings.begin(); it != bindings.end(); ++it) {
        QString key;
        switch (it.key()) {
        case GameAction::Split: key = "split"; break;
        case GameAction::Eject: key = "eject"; break;
        case GameAction::ToggleDebug: key = "toggleDebug"; break;
        case GameAction::ToggleControlMode: key = "toggleControlMode"; break;
        }
        o[key] = it.value();
    }
    return o;
}

KeyBinding KeyBinding::fromJson(const QJsonObject& o) {
    KeyBinding kb;
    auto setIfPresent = [&](const QString& name, GameAction action) {
        if (o.contains(name)) {
            kb.bindings[action] = o.value(name).toInt();
        }
    };
    setIfPresent("split", GameAction::Split);
    setIfPresent("eject", GameAction::Eject);
    setIfPresent("toggleDebug", GameAction::ToggleDebug);
    setIfPresent("toggleControlMode", GameAction::ToggleControlMode);
    return kb;
}
