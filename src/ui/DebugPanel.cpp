#include "DebugPanel.h"
#include "engine/GameEngine.h"
#include <QVBoxLayout>
#include <QHBoxLayout>

DebugPanel::DebugPanel(QWidget* parent) : QWidget(parent) {
    setFixedSize(200, 320);
    setStyleSheet(R"(
        QWidget {
            background: rgba(0,0,0,0.85);
            border: 1px solid rgba(255,255,255,0.15);
            border-radius: 8px;
        }
        QLabel {
            color: #ccc;
            font-size: 11px;
            background: transparent;
        }
        QSlider::groove:horizontal {
            height: 6px;
            background: rgba(255,255,255,0.1);
            border-radius: 3px;
        }
        QSlider::handle:horizontal {
            width: 14px;
            height: 14px;
            margin: -4px 0;
            background: #FFD700;
            border-radius: 7px;
        }
        QSlider::sub-page:horizontal {
            background: rgba(255,215,0,0.3);
            border-radius: 3px;
        }
        QCheckBox {
            color: #ccc;
            font-size: 12px;
        }
        QCheckBox::indicator {
            width: 16px;
            height: 16px;
        }
        QPushButton {
            color: #ccc;
            font-size: 11px;
            background: rgba(255,255,255,0.08);
            border: 1px solid rgba(255,255,255,0.15);
            border-radius: 4px;
            padding: 4px 8px;
        }
        QPushButton:hover {
            color: white;
            background: rgba(255,255,255,0.15);
        }
    )");

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(10, 8, 10, 8);
    layout->setSpacing(6);

    auto* title = new QLabel("Debug Panel (F3)", this);
    title->setStyleSheet("color: #FFD700; font-size: 13px; font-weight: bold;");
    title->setAlignment(Qt::AlignCenter);
    layout->addWidget(title);

    // Mass slider
    auto* massLabel = new QLabel("Mass: 10", this);
    massLabel->setAlignment(Qt::AlignCenter);
    m_massValueLabel = massLabel;
    layout->addWidget(massLabel);

    m_massSlider = new QSlider(Qt::Horizontal, this);
    m_massSlider->setRange(10, 50000);
    m_massSlider->setValue(10);
    layout->addWidget(m_massSlider);

    connect(m_massSlider, &QSlider::valueChanged, this, [this](int val) {
        m_massValueLabel->setText(QString("Mass: %1").arg(val));
        emit massChanged((float)val);
    });

    // God mode
    m_godModeCheck = new QCheckBox("God Mode", this);
    layout->addWidget(m_godModeCheck);
    connect(m_godModeCheck, &QCheckBox::toggled, this, &DebugPanel::godModeToggled);

    // Speed boost
    m_speedBoostCheck = new QCheckBox("Speed Boost (3x)", this);
    layout->addWidget(m_speedBoostCheck);
    connect(m_speedBoostCheck, &QCheckBox::toggled, this, &DebugPanel::speedBoostToggled);

    layout->addSpacing(4);

    // Buttons
    auto* spawnFoodBtn = new QPushButton("Spawn 100 Food", this);
    layout->addWidget(spawnFoodBtn);
    connect(spawnFoodBtn, &QPushButton::clicked, this, [this]() {
        emit spawnFoodRequested(100);
    });

    auto* spawnBigBtn = new QPushButton("Spawn Big Bean", this);
    layout->addWidget(spawnBigBtn);
    connect(spawnBigBtn, &QPushButton::clicked, this, &DebugPanel::spawnBigBeanRequested);

    auto* teleportBtn = new QPushButton("Teleport to Center", this);
    layout->addWidget(teleportBtn);
    connect(teleportBtn, &QPushButton::clicked, this, &DebugPanel::teleportToCenterRequested);

    auto* resetBtn = new QPushButton("Reset", this);
    resetBtn->setStyleSheet("QPushButton { color: #e74c3c; } QPushButton:hover { color: #ff6b6b; }");
    layout->addWidget(resetBtn);
    connect(resetBtn, &QPushButton::clicked, this, [this]() {
        m_massSlider->setValue(10);
        m_godModeCheck->setChecked(false);
        m_speedBoostCheck->setChecked(false);
        emit resetRequested();
    });

    layout->addStretch();
}
