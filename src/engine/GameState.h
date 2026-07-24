#ifndef GAMESTATE_H
#define GAMESTATE_H

#include <QObject>

enum class GameStateEnum {
    Menu,
    Playing,
    Paused,
    GameOver
};

class GameState : public QObject {
    Q_OBJECT
public:
    explicit GameState(QObject* parent = nullptr);

    GameStateEnum current() const { return m_state; }
    void set(GameStateEnum s);

signals:
    void changed(GameStateEnum newState);

private:
    GameStateEnum m_state = GameStateEnum::Menu;
};

#endif // GAMESTATE_H
