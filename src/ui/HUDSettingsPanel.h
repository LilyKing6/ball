#ifndef HUDSETTINGSPANEL_H
#define HUDSETTINGSPANEL_H

#include <QWidget>
#include <QComboBox>
#include <QSlider>
#include <QLabel>
#include <QPushButton>
#include <QTabWidget>
#include <QCheckBox>

class HUDSettingsPanel : public QWidget {
    Q_OBJECT
public:
    explicit HUDSettingsPanel(QWidget* parent = nullptr);

signals:
    void qualityChanged(int preset);
    void fpsLimitChanged(int fps);
    void hudScaleChanged(float scale);
    void skillButtonSizeChanged(int size);
    void openFullSettings();
    void quitGame();
    void closed();

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    QTabWidget* m_tabs;

    // 游戏设置 tab
    QComboBox* m_qualityCombo;
    QComboBox* m_fpsCombo;
    QSlider* m_scaleSlider;
    QLabel* m_scaleValue;
    QComboBox* m_buttonSizeCombo;

    // 操作设置 tab - 控制系统
    QComboBox* m_controlModeCombo;
    QComboBox* m_joystickPosCombo;
    QCheckBox* m_joystickFixedCheck;
    QSlider* m_joystickRadiusSlider;
    QLabel* m_joystickRadiusValue;
    QSlider* m_joystickDeadzoneSlider;
    QLabel* m_joystickDeadzoneValue;
    QSlider* m_sensitivitySlider;
    QLabel* m_sensValue;

    QWidget* createGameSettingsTab();
    QWidget* createControlSettingsTab();
    void loadFromConfig();
};

#endif // HUDSETTINGSPANEL_H
