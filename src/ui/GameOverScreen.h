#ifndef GAMEOVERSCREEN_H
#define GAMEOVERSCREEN_H

#include "SubWindow.h"
#include "record/GameRecord.h"
#include <QLabel>
#include <QPushButton>

class GameOverScreen : public SubWindow {
    Q_OBJECT
public:
    explicit GameOverScreen(QWidget* parent = nullptr);

    void showStats(const GameRecord& rec, int eloChange, bool victory = false);

signals:
    void playAgain();
    void returnToMenu();

private:
    QLabel* m_titleLabel;
    QLabel* m_rankValue;
    QLabel* m_massValue;
    QLabel* m_timeValue;
    QLabel* m_killsValue;
    QLabel* m_foodValue;
    QLabel* m_eloValue;
};

#endif // GAMEOVERSCREEN_H
