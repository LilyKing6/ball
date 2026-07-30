#include "SettingsWindow.h"
#include "Style.h"
#include "audio/AudioManager.h"
#include "util/Config.h"
#include "input/InputManager.h"
#include <QHBoxLayout>
#include <QLabel>
#include <QVBoxLayout>
#include <QFrame>
#include <QScrollArea>
#include <QWidget>
#include <QKeyEvent>

// 预设分辨率定义
struct ResolutionPreset {
    const char* label;
    int width;
    int height;
};

static const ResolutionPreset kResolutions[] = {
    {"1280×720 (HD)",       1280, 720},
    {"1920×1080 (FHD)",     1920, 1080},
    {"2560×1440 (2K QHD)",  2560, 1440},
    {"3840×2160 (4K UHD)",  3840, 2160},
};

static const char* kDisplayModes[] = {
    "窗口模式",
    "无边框窗口",
    "全屏",
};

// 辅助：创建带标题的分组卡片
static QFrame* makeSectionCard(QWidget* parent, const QString& title, QLayout* content) {
    auto* card = new QFrame(parent);
    card->setStyleSheet(Style::cardStyle());

    auto* layout = new QVBoxLayout(card);
    layout->setContentsMargins(16, 14, 16, 16);
    layout->setSpacing(10);

    auto* titleLabel = new QLabel(title, card);
    titleLabel->setStyleSheet(Style::sectionTitleStyle());
    layout->addWidget(titleLabel);

    layout->addLayout(content);
    return card;
}

// 辅助：创建带标签的滑块行
static QHBoxLayout* makeSliderRow(QWidget* parent, const QString& label, QSlider*& slider,
                                  QLabel*& valueLabel, int min, int max, int value, const QColor& color) {
    auto* row = new QHBoxLayout();
    row->setSpacing(10);

    auto* lbl = new QLabel(label, parent);
    lbl->setStyleSheet("font-size: 13px; color: #ccc; background: transparent;");
    lbl->setFixedWidth(90);
    row->addWidget(lbl);

    slider = new QSlider(Qt::Horizontal, parent);
    slider->setRange(min, max);
    slider->setValue(value);
    slider->setStyleSheet(Style::sliderStyle(color));
    row->addWidget(slider, 1);

    valueLabel = new QLabel(parent);
    valueLabel->setStyleSheet(QString("font-size: 13px; color: %1; background: transparent; min-width: 40px;").arg(color.name()));
    valueLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    row->addWidget(valueLabel);

    return row;
}

SettingsWindow::SettingsWindow(QWidget* parent)
    : SubWindow("设置", parent, 520, 600) {

    auto* l = contentLayout();
    l->setSpacing(0);
    l->setContentsMargins(0, 0, 0, 0);

    // 创建滚动区域
    auto* scroll = new QScrollArea(this);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scroll->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    scroll->setStyleSheet(R"(
        QScrollArea { background: transparent; border: none; }
        QScrollBar:vertical {
            background: rgba(0,0,0,0.2);
            width: 8px;
            border-radius: 4px;
        }
        QScrollBar::handle:vertical {
            background: rgba(255,255,255,0.25);
            border-radius: 4px;
            min-height: 40px;
        }
        QScrollBar::handle:vertical:hover { background: rgba(255,255,255,0.35); }
        QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0px; }
    )");

    auto* contentWidget = new QWidget(this);
    contentWidget->setStyleSheet("background: transparent;");
    auto* contentLayout = new QVBoxLayout(contentWidget);
    contentLayout->setSpacing(14);
    contentLayout->setContentsMargins(22, 18, 22, 22);

    // ========== 视频设置 ==========
    auto* videoContent = new QVBoxLayout();
    videoContent->setSpacing(10);
    videoContent->setContentsMargins(0, 0, 0, 0);

    auto* resLabel = new QLabel("分辨率", this);
    resLabel->setStyleSheet("font-size: 13px; color: #ccc; background: transparent;");
    videoContent->addWidget(resLabel);

    m_resolutionCombo = new QComboBox(this);
    m_resolutionCombo->setStyleSheet(Style::comboBoxStyle());
    for (const auto& r : kResolutions) {
        m_resolutionCombo->addItem(r.label);
    }
    m_resolutionCombo->setCurrentIndex(Config::instance().resolutionIndex);
    videoContent->addWidget(m_resolutionCombo);

    connect(m_resolutionCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
        this, [this](int idx) {
            Config::instance().resolutionIndex = idx;
            emit resolutionChanged(idx);
        });

    auto* modeLabel = new QLabel("显示模式", this);
    modeLabel->setStyleSheet("font-size: 13px; color: #ccc; background: transparent;");
    videoContent->addWidget(modeLabel);

    m_displayModeCombo = new QComboBox(this);
    m_displayModeCombo->setStyleSheet(Style::comboBoxStyle());
    for (const auto& m : kDisplayModes) {
        m_displayModeCombo->addItem(m);
    }
    m_displayModeCombo->setCurrentIndex(Config::instance().displayMode);
    videoContent->addWidget(m_displayModeCombo);

    connect(m_displayModeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
        this, [this](int idx) {
            Config::instance().displayMode = idx;
            emit displayModeChanged(idx);
        });

    contentLayout->addWidget(makeSectionCard(this, "🎮 视频设置", videoContent));

    // ========== 控制设置 ==========
    auto* ctrlContent = new QVBoxLayout();
    ctrlContent->setSpacing(10);
    ctrlContent->setContentsMargins(0, 0, 0, 0);

    auto* sensRow = makeSliderRow(this, "鼠标灵敏度", m_sensitivitySlider, m_sensValue,
                                  50, 200, 100, Style::accentGold());
    ctrlContent->addLayout(sensRow);

    connect(m_sensitivitySlider, &QSlider::valueChanged, this, [this](int v) {
        m_sensValue->setText(QString("%1x").arg(v / 100.0, 0, 'f', 1));
        emit sensitivityChanged(v / 100.0f);
    });

    contentLayout->addWidget(makeSectionCard(this, "🖱 控制设置", ctrlContent));

    // ========== 音频设置 ==========
    auto* audioContent = new QVBoxLayout();
    audioContent->setSpacing(10);
    audioContent->setContentsMargins(0, 0, 0, 0);

    auto* sfxRow = makeSliderRow(this, "音效音量", m_sfxVolumeSlider, m_sfxVolumeValue,
                                 0, 100, static_cast<int>(Config::instance().sfxVolume * 100),
                                 Style::accentGreen());
    audioContent->addLayout(sfxRow);

    connect(m_sfxVolumeSlider, &QSlider::valueChanged, this, [this](int v) {
        m_sfxVolumeValue->setText(QString("%1%").arg(v));
        float vol = v / 100.0f;
        AudioManager::instance().setSfxVolume(vol);
        Config::instance().sfxVolume = vol;
    });

    auto* bgmRow = makeSliderRow(this, "背景音乐音量", m_bgmVolumeSlider, m_bgmVolumeValue,
                                 0, 100, static_cast<int>(Config::instance().bgmVolume * 100),
                                 Style::accentBlue());
    audioContent->addLayout(bgmRow);

    connect(m_bgmVolumeSlider, &QSlider::valueChanged, this, [this](int v) {
        m_bgmVolumeValue->setText(QString("%1%").arg(v));
        float vol = v / 100.0f;
        AudioManager::instance().setBgmVolume(vol);
        Config::instance().bgmVolume = vol;
    });

    contentLayout->addWidget(makeSectionCard(this, "🔊 音频设置", audioContent));

    // ========== 操作键位 ==========
    auto* keyContent = new QVBoxLayout();
    keyContent->setSpacing(10);
    keyContent->setContentsMargins(0, 0, 0, 0);

    auto addKeyRow = [&](GameAction action, const QString& label) {
        auto* row = new QHBoxLayout();
        row->setSpacing(10);

        auto* lbl = new QLabel(label, this);
        lbl->setStyleSheet("font-size: 13px; color: #ccc; background: transparent;");
        lbl->setFixedWidth(80);
        row->addWidget(lbl);

        auto* btn = new QPushButton(this);
        btn->setStyleSheet(Style::iconButtonStyle());
        btn->setFixedWidth(100);
        row->addWidget(btn);
        m_keyButtons[action] = btn;

        connect(btn, &QPushButton::clicked, this, [this, action, btn]() {
            startCaptureKey(action, btn);
        });

        keyContent->addLayout(row);
    };

    addKeyRow(GameAction::Split, "⚔ 分裂");
    addKeyRow(GameAction::Eject, "💨 吐球");
    addKeyRow(GameAction::ToggleDebug, "🔍 调试");
    addKeyRow(GameAction::ToggleControlMode, "🔄 切换模式");

    updateKeyLabels();

    contentLayout->addWidget(makeSectionCard(this, "⌨ 操作键位", keyContent));

    contentLayout->addStretch();

    scroll->setWidget(contentWidget);
    l->addWidget(scroll);

    // 安装事件过滤器以捕获按键绑定输入
    this->installEventFilter(this);
}

void SettingsWindow::startCaptureKey(GameAction action, QPushButton* btn) {
    m_capturingAction = action;
    m_capturingKey = true;
    btn->setText("按任意键...");
    btn->setStyleSheet(Style::iconButtonStyle());
    this->setFocus();
    this->grabKeyboard();
}

void SettingsWindow::updateKeyLabels() {
    auto& kb = Config::instance().keyBindings;
    for (auto it = m_keyButtons.begin(); it != m_keyButtons.end(); ++it) {
        it.value()->setText(kb.keyName(it.key()));
        it.value()->setStyleSheet(Style::iconButtonStyle());
    }
}

void SettingsWindow::applyKeyBindings() {
    auto& cfg = Config::instance();
    InputManager::instance().setBinding(cfg.keyBindings);
}

bool SettingsWindow::eventFilter(QObject* watched, QEvent* event) {
    if (m_capturingKey && event->type() == QEvent::KeyPress) {
        auto* ke = dynamic_cast<QKeyEvent*>(event);
        if (!ke) return SubWindow::eventFilter(watched, event);
        int k = ke->key();
        if (k == Qt::Key_Escape || k == Qt::Key_Backspace) {
            // 取消捕获
            m_capturingKey = false;
            this->releaseKeyboard();
            updateKeyLabels();
            return true;
        }
        // 设置新键位
        Config::instance().keyBindings.setKey(m_capturingAction, k);
        m_capturingKey = false;
        this->releaseKeyboard();
        updateKeyLabels();
        applyKeyBindings();
        return true;
    }
    return SubWindow::eventFilter(watched, event);
}
