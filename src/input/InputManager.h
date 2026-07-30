#ifndef INPUTMANAGER_H
#define INPUTMANAGER_H

#include <QObject>
#include <QKeyEvent>
#include "input/KeyBinding.h"

class GameEngine;

// InputManager
//   将键盘事件映射为游戏操作，并调用 GameEngine 对应方法。
//   键位绑定从 Config 读取，可在设置中修改。
class InputManager : public QObject {
    Q_OBJECT
public:
    static InputManager& instance();

    void setBinding(const KeyBinding& binding) { m_binding = binding; }
    const KeyBinding& binding() const { return m_binding; }

    // 处理按键按下，若该键绑定了操作则执行并返回 true
    bool handleKeyPress(QKeyEvent* event, GameEngine* engine);
    // 处理按键释放，若该键绑定了操作则返回 true（不执行动作）
    bool handleKeyRelease(QKeyEvent* event, GameEngine* engine);

private:
    explicit InputManager(QObject* parent = nullptr);
    KeyBinding m_binding;
};

#endif // INPUTMANAGER_H
