#ifndef DEBUGPANEL_H
#define DEBUGPANEL_H

#include <QWidget>
#include <QSlider>
#include <QCheckBox>
#include <QPushButton>
#include <QLabel>

class GameEngine;

class DebugPanel : public QWidget {
    Q_OBJECT
public:
    explicit DebugPanel(QWidget* parent = nullptr);

    void setEngine(GameEngine* engine) { m_engine = engine; }

signals:
    void massChanged(float mass);
    void godModeToggled(bool enabled);
    void speedBoostToggled(bool enabled);
    void spawnFoodRequested(int count);
    void spawnBigBeanRequested();
    void teleportToCenterRequested();
    void resetRequested();

private:
    GameEngine* m_engine = nullptr;
    QSlider* m_massSlider;
    QLabel* m_massValueLabel;
    QCheckBox* m_godModeCheck;
    QCheckBox* m_speedBoostCheck;
};

#endif // DEBUGPANEL_H
