#include "InputManager.h"
#include "engine/GameEngine.h"

InputManager& InputManager::instance() {
    static InputManager s;
    return s;
}

InputManager::InputManager(QObject* parent) : QObject(parent) {
    m_binding = KeyBinding();
}

bool InputManager::handleKeyPress(QKeyEvent* event, GameEngine* engine) {
    if (!engine || event->isAutoRepeat()) return false;

    int k = event->key();
    if (k == m_binding.key(GameAction::Split)) {
        engine->splitLocalPlayer();
        return true;
    }
    if (k == m_binding.key(GameAction::Eject)) {
        engine->ejectFromLocalPlayer();
        return true;
    }
    return false;
}

bool InputManager::handleKeyRelease(QKeyEvent* event, GameEngine* engine) {
    Q_UNUSED(engine)
    int k = event->key();
    // 目前只有 Eject 需要释放检测（用于 QPainterGLWidget 的 eKeyDown）
    // GLWidget 不依赖释放事件执行动作
    if (k == m_binding.key(GameAction::Eject)) {
        return true;
    }
    return false;
}
