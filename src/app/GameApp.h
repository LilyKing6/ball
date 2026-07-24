#ifndef GAMEAPP_H
#define GAMEAPP_H

#include <QObject>

class GameEngine;
class MainWindow;
class QApplication;

class GameApp : public QObject {
    Q_OBJECT
public:
    GameApp(int argc, char* argv[]);
    ~GameApp();

    int run();

    GameEngine* engine() const { return m_engine; }
    MainWindow* window() const { return m_window; }

private:
    QApplication* m_app = nullptr;
    GameEngine* m_engine = nullptr;
    MainWindow* m_window = nullptr;
};

#endif // GAMEAPP_H
