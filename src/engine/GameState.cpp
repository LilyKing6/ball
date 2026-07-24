#include "GameState.h"

GameState::GameState(QObject* parent) : QObject(parent) {}

void GameState::set(GameStateEnum s) {
    if (m_state != s) {
        m_state = s;
        emit changed(s);
    }
}
