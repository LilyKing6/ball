#ifndef SETTINGSWINDOW_H
#define SETTINGSWINDOW_H

#include "SubWindow.h"
#include <QSlider>
#include <QLabel>
#include <QComboBox>

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
};

#endif // SETTINGSWINDOW_H
