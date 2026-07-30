#ifndef SETTINGSWINDOW_H
#define SETTINGSWINDOW_H

#include "SubWindow.h"
#include "input/KeyBinding.h"
#include <QSlider>
#include <QLabel>
#include <QComboBox>
#include <QPushButton>
#include <QMap>

class SettingsWindow : public SubWindow {
    Q_OBJECT
public:
    explicit SettingsWindow(QWidget* parent = nullptr);

signals:
    void sensitivityChanged(float value);
    void resolutionChanged(int index);
    void displayModeChanged(int mode);

private:
    QSlider* m_sensitivitySlider;
    QLabel* m_sensValue;
    QSlider* m_sfxVolumeSlider;
    QLabel* m_sfxVolumeValue;
    QSlider* m_bgmVolumeSlider;
    QLabel* m_bgmVolumeValue;
    QComboBox* m_resolutionCombo;
    QComboBox* m_displayModeCombo;

    // 键位绑定编辑
    QMap<GameAction, QPushButton*> m_keyButtons;
    GameAction m_capturingAction = GameAction::Split;
    bool m_capturingKey = false;

    void startCaptureKey(GameAction action, QPushButton* btn);
    void updateKeyLabels();
    void applyKeyBindings();

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;
};

#endif // SETTINGSWINDOW_H
