#include "HUDSettingsPanel.h"
#include "util/Config.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPainter>
#include <QScrollArea>
#include <QFrame>

HUDSettingsPanel::HUDSettingsPanel(QWidget* parent) : QWidget(parent) {
    setFixedSize(360, 520);
    // 作为独立的 Tool 窗口，不会拦截 GLWidget 焦点，也不会触发其重绘
    setWindowFlags(Qt::Tool | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
    setAttribute(Qt::WA_TranslucentBackground);

    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(10, 10, 10, 10);
    mainLayout->setSpacing(0);

    // 标题
    auto* title = new QLabel("快捷设置", this);
    title->setStyleSheet("color: #FFD700; font-size: 14px; font-weight: bold;");
    title->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(title);
    mainLayout->addSpacing(8);

    // 标签页
    m_tabs = new QTabWidget(this);
    m_tabs->setStyleSheet(R"(
        QTabWidget::pane {
            background: transparent;
            border: none;
        }
        QTabBar::tab {
            color: #888;
            background: transparent;
            border: none;
            padding: 6px 16px;
            font-size: 12px;
        }
        QTabBar::tab:selected {
            color: #FFD700;
            border-bottom: 2px solid #FFD700;
        }
        QTabBar::tab:hover {
            color: #ccc;
        }
    )");
    m_tabs->addTab(createGameSettingsTab(), "游戏设置");
    m_tabs->addTab(createControlSettingsTab(), "操作设置");
    mainLayout->addWidget(m_tabs);

    mainLayout->addSpacing(8);

    // 底部按钮
    auto* btnRow = new QHBoxLayout();
    btnRow->setSpacing(8);

    auto* fullSettingsBtn = new QPushButton("完整设置...", this);
    fullSettingsBtn->setStyleSheet(R"(
        QPushButton { color: #ccc; background: rgba(255,255,255,0.08); border: 1px solid rgba(255,255,255,0.15); border-radius: 4px; padding: 6px 12px; font-size: 12px; }
        QPushButton:hover { color: white; background: rgba(255,255,255,0.15); }
    )");
    fullSettingsBtn->setCursor(Qt::PointingHandCursor);
    btnRow->addWidget(fullSettingsBtn);

    btnRow->addStretch();

    auto* quitBtn = new QPushButton("退出游戏", this);
    quitBtn->setStyleSheet(R"(
        QPushButton { color: #e74c3c; background: rgba(231,76,60,0.1); border: 1px solid rgba(231,76,60,0.3); border-radius: 4px; padding: 6px 12px; font-size: 12px; }
        QPushButton:hover { color: #ff6b6b; background: rgba(231,76,60,0.2); }
    )");
    quitBtn->setCursor(Qt::PointingHandCursor);
    btnRow->addWidget(quitBtn);

    mainLayout->addLayout(btnRow);

    connect(fullSettingsBtn, &QPushButton::clicked, this, [this]() {
        hide();
        emit openFullSettings();
    });
    connect(quitBtn, &QPushButton::clicked, this, [this]() {
        hide();
        emit quitGame();
    });

    loadFromConfig();
}

QWidget* HUDSettingsPanel::createGameSettingsTab() {
    auto* page = new QWidget(this);
    auto* l = new QVBoxLayout(page);
    l->setContentsMargins(8, 8, 8, 8);
    l->setSpacing(6);

    auto makeLabel = [&](const char* text) {
        auto* lb = new QLabel(text, page);
        lb->setStyleSheet("color: #ccc; font-size: 12px;");
        l->addWidget(lb);
    };

    auto comboStyle = R"(
        QComboBox { background: rgba(255,255,255,0.08); color: #ccc; border: 1px solid rgba(255,255,255,0.15); border-radius: 4px; padding: 4px 8px; font-size: 12px; }
        QComboBox:hover { border-color: rgba(255,255,255,0.3); }
        QComboBox QAbstractItemView { background: #1a1a2e; color: #ccc; border: 1px solid rgba(255,255,255,0.15); selection-background-color: rgba(255,215,0,0.2); }
    )";

    makeLabel("画面品质");
    m_qualityCombo = new QComboBox(page);
    m_qualityCombo->addItems({"低", "中", "高"});
    m_qualityCombo->setStyleSheet(comboStyle);
    l->addWidget(m_qualityCombo);

    makeLabel("帧率限制");
    m_fpsCombo = new QComboBox(page);
    m_fpsCombo->addItems({"无限制", "30 FPS", "60 FPS", "120 FPS"});
    m_fpsCombo->setStyleSheet(comboStyle);
    l->addWidget(m_fpsCombo);

    makeLabel("HUD 缩放");
    auto* scaleRow = new QHBoxLayout();
    m_scaleSlider = new QSlider(Qt::Horizontal, page);
    m_scaleSlider->setRange(70, 150);
    m_scaleSlider->setValue(100);
    m_scaleSlider->setStyleSheet(R"(
        QSlider::groove:horizontal { height: 4px; background: #333; border-radius: 2px; }
        QSlider::handle:horizontal { width: 12px; height: 12px; margin: -4px 0; background: #FFD700; border-radius: 6px; }
    )");
    scaleRow->addWidget(m_scaleSlider);

    m_scaleValue = new QLabel("1.0x", page);
    m_scaleValue->setStyleSheet("color: #FFD700; font-size: 12px; min-width: 32px;");
    scaleRow->addWidget(m_scaleValue);
    l->addLayout(scaleRow);

    makeLabel("技能按钮大小");
    m_buttonSizeCombo = new QComboBox(page);
    m_buttonSizeCombo->addItems({"小", "中", "大"});
    m_buttonSizeCombo->setStyleSheet(comboStyle);
    l->addWidget(m_buttonSizeCombo);

    l->addStretch();

    // 连接
    connect(m_qualityCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int idx) {
        Config::instance().hudQualityPreset = idx;
        emit qualityChanged(idx);
    });
    connect(m_fpsCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int idx) {
        int fps = 0;
        switch (idx) { case 1: fps = 30; break; case 2: fps = 60; break; case 3: fps = 120; break; }
        Config::instance().hudFpsLimit = fps;
        emit fpsLimitChanged(fps);
    });
    connect(m_scaleSlider, &QSlider::valueChanged, this, [this](int v) {
        float s = v / 100.0f;
        m_scaleValue->setText(QString("%1x").arg(s, 0, 'f', 1));
        Config::instance().hudScale = s;
        emit hudScaleChanged(s);
    });
    connect(m_buttonSizeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int idx) {
        Config::instance().hudSkillButtonSize = idx;
        emit skillButtonSizeChanged(idx);
    });

    return page;
}

QWidget* HUDSettingsPanel::createControlSettingsTab() {
    auto* scroll = new QScrollArea(this);
    scroll->setWidgetResizable(true);
    scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scroll->setStyleSheet(R"(
        QScrollArea { background: transparent; border: none; }
        QScrollBar:vertical { background: rgba(255,255,255,0.05); width: 6px; }
        QScrollBar::handle:vertical { background: rgba(255,215,0,0.4); border-radius: 3px; }
        QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { background: none; }
    )");

    auto* page = new QWidget();
    page->setStyleSheet("background: transparent;");
    auto* l = new QVBoxLayout(page);
    l->setContentsMargins(8, 8, 8, 8);
    l->setSpacing(6);

    auto makeLabel = [&](const char* text) {
        auto* lb = new QLabel(text, page);
        lb->setStyleSheet("color: #ccc; font-size: 12px;");
        l->addWidget(lb);
    };

    auto makeSection = [&](const char* text) {
        auto* lb = new QLabel(text, page);
        lb->setStyleSheet("color: #FFD700; font-size: 13px; font-weight: bold; margin-top: 6px;");
        l->addWidget(lb);
        auto* sep = new QFrame(page);
        sep->setFrameShape(QFrame::HLine);
        sep->setStyleSheet("background: rgba(255,215,0,0.3); max-height: 1px;");
        l->addWidget(sep);
    };

    auto comboStyle = R"(
        QComboBox { background: rgba(255,255,255,0.08); color: #ccc; border: 1px solid rgba(255,255,255,0.15); border-radius: 4px; padding: 4px 8px; font-size: 12px; }
        QComboBox:hover { border-color: rgba(255,255,255,0.3); }
        QComboBox QAbstractItemView { background: #1a1a2e; color: #ccc; border: 1px solid rgba(255,255,255,0.15); selection-background-color: rgba(255,215,0,0.2); }
    )";
    auto sliderStyle = R"(
        QSlider::groove:horizontal { height: 4px; background: #333; border-radius: 2px; }
        QSlider::handle:horizontal { width: 12px; height: 12px; margin: -4px 0; background: #FFD700; border-radius: 6px; }
    )";

    // === 控制方式 ===
    makeSection("控制方式");

    makeLabel("控制模式");
    m_controlModeCombo = new QComboBox(page);
    m_controlModeCombo->addItems({"鼠标游标", "虚拟摇杆", "双模式（Shift 切换）"});
    m_controlModeCombo->setStyleSheet(comboStyle);
    l->addWidget(m_controlModeCombo);

    makeLabel("摇杆位置");
    m_joystickPosCombo = new QComboBox(page);
    m_joystickPosCombo->addItems({"左下", "中下", "右下", "自定义"});
    m_joystickPosCombo->setStyleSheet(comboStyle);
    l->addWidget(m_joystickPosCombo);

    makeLabel("摇杆类型");
    m_joystickFixedCheck = new QCheckBox("固定位置（关闭=按下处为摇杆中心）", page);
    m_joystickFixedCheck->setStyleSheet("color: #ccc; font-size: 12px;");
    l->addWidget(m_joystickFixedCheck);

    makeLabel("摇杆半径（控制游标最大偏移距离）");
    auto* rRow = new QHBoxLayout();
    m_joystickRadiusSlider = new QSlider(Qt::Horizontal, page);
    m_joystickRadiusSlider->setRange(200, 800);
    m_joystickRadiusSlider->setStyleSheet(sliderStyle);
    rRow->addWidget(m_joystickRadiusSlider);
    m_joystickRadiusValue = new QLabel("500", page);
    m_joystickRadiusValue->setStyleSheet("color: #FFD700; font-size: 12px; min-width: 32px;");
    rRow->addWidget(m_joystickRadiusValue);
    l->addLayout(rRow);

    makeLabel("防误触阈值（按下后位移小于此值不激活）");
    auto* dzRow = new QHBoxLayout();
    m_joystickDeadzoneSlider = new QSlider(Qt::Horizontal, page);
    m_joystickDeadzoneSlider->setRange(0, 30);
    m_joystickDeadzoneSlider->setStyleSheet(sliderStyle);
    dzRow->addWidget(m_joystickDeadzoneSlider);
    m_joystickDeadzoneValue = new QLabel("10", page);
    m_joystickDeadzoneValue->setStyleSheet("color: #FFD700; font-size: 12px; min-width: 32px;");
    dzRow->addWidget(m_joystickDeadzoneValue);
    l->addLayout(dzRow);

    // === 灵敏度 ===
    makeSection("灵敏度");

    makeLabel("摇杆灵敏度");
    auto* sensRow = new QHBoxLayout();
    m_sensitivitySlider = new QSlider(Qt::Horizontal, page);
    m_sensitivitySlider->setRange(50, 200);
    m_sensitivitySlider->setValue(100);
    m_sensitivitySlider->setStyleSheet(sliderStyle);
    sensRow->addWidget(m_sensitivitySlider);
    m_sensValue = new QLabel("1.0x", page);
    m_sensValue->setStyleSheet("color: #FFD700; font-size: 12px; min-width: 32px;");
    sensRow->addWidget(m_sensValue);
    l->addLayout(sensRow);

    // === 键位说明 ===
    makeSection("键位");

    auto* keyInfo = new QLabel("移动: 鼠标 / 摇杆\n分裂: Space\n吐球 / 聚球: E\n暂停: Esc\nHybrid 切换: Shift", page);
    keyInfo->setStyleSheet("color: #999; font-size: 11px; padding: 4px;");
    l->addWidget(keyInfo);

    l->addStretch();

    // === 信号连接 ===
    connect(m_controlModeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int idx) {
        Config::instance().controlMode = idx;
    });
    connect(m_joystickPosCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int idx) {
        Config::instance().joystickPosition = idx;
    });
    connect(m_joystickFixedCheck, &QCheckBox::toggled, this, [this](bool v) {
        Config::instance().joystickFixed = v;
    });
    connect(m_joystickRadiusSlider, &QSlider::valueChanged, this, [this](int v) {
        m_joystickRadiusValue->setText(QString::number(v));
        Config::instance().joystickRadius = static_cast<float>(v);
    });
    connect(m_joystickDeadzoneSlider, &QSlider::valueChanged, this, [this](int v) {
        m_joystickDeadzoneValue->setText(QString::number(v));
        Config::instance().joystickDeadzone = static_cast<float>(v);
    });
    connect(m_sensitivitySlider, &QSlider::valueChanged, this, [this](int v) {
        m_sensValue->setText(QString("%1x").arg(v / 100.0, 0, 'f', 1));
    });

    scroll->setWidget(page);
    return scroll;
}

void HUDSettingsPanel::loadFromConfig() {
    auto& cfg = Config::instance();
    m_qualityCombo->setCurrentIndex(cfg.hudQualityPreset);
    m_scaleSlider->setValue(static_cast<int>(cfg.hudScale * 100));
    m_buttonSizeCombo->setCurrentIndex(cfg.hudSkillButtonSize);

    int fpsIdx = 0;
    switch (cfg.hudFpsLimit) { case 30: fpsIdx = 1; break; case 60: fpsIdx = 2; break; case 120: fpsIdx = 3; break; }
    m_fpsCombo->setCurrentIndex(fpsIdx);

    // 控制设置
    if (m_controlModeCombo) m_controlModeCombo->setCurrentIndex(cfg.controlMode);
    if (m_joystickPosCombo) m_joystickPosCombo->setCurrentIndex(cfg.joystickPosition);
    if (m_joystickFixedCheck) m_joystickFixedCheck->setChecked(cfg.joystickFixed);
    if (m_joystickRadiusSlider) m_joystickRadiusSlider->setValue(static_cast<int>(cfg.joystickRadius));
    if (m_joystickDeadzoneSlider) m_joystickDeadzoneSlider->setValue(static_cast<int>(cfg.joystickDeadzone));
}

void HUDSettingsPanel::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.setPen(QPen(QColor(255, 255, 255, 30), 1));
    p.setBrush(QColor(15, 20, 40, 245));
    p.drawRoundedRect(rect().adjusted(1, 1, -1, -1), 10, 10);
}
