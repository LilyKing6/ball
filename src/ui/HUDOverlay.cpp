#include "HUDOverlay.h"
#include "Style.h"
#include "HUDSettingsPanel.h"
#include "ui/LeaderboardWidget.h"
#include "util/Config.h"
#include <QPainter>
#include <QGraphicsOpacityEffect>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QMouseEvent>
#include <QApplication>

HUDOverlay::HUDOverlay(QWidget* parent) : QWidget(parent) {
    // HUDOverlay 是控制器，本身不显示任何内容
    setAttribute(Qt::WA_TransparentForMouseEvents, true);
    setStyleSheet("background: transparent;");
    resize(1, 1);

    createTopBar();
    createSkillButtons();
    createSettingsPanel();

    // 缩圈警告标签（顶栏下方居中）
    m_zoneWarningLabel = new QLabel(parent);
    m_zoneWarningLabel->setStyleSheet(R"(
        QLabel {
            background: rgba(200,30,30,0.88);
            color: white;
            border: 1px solid #ff6666;
            border-radius: 8px;
            padding: 8px 22px;
            font-size: 15px;
            font-weight: bold;
        }
    )");
    m_zoneWarningLabel->setAlignment(Qt::AlignCenter);
    m_zoneWarningLabel->hide();

    // 死亡遮罩
    m_deathLabel = new QLabel(parent);
    m_deathLabel->setStyleSheet(R"(
        QLabel {
            background: rgba(10,10,20,0.88);
            color: #e74c3c;
            border: 2px solid rgba(231,76,60,0.6);
            border-radius: 14px;
            padding: 24px 44px;
            font-size: 24px;
            font-weight: bold;
        }
    )");
    m_deathLabel->setAlignment(Qt::AlignCenter);
    m_deathLabel->hide();

    // Toast
    m_toastLabel = new QLabel(parent);
    m_toastLabel->setStyleSheet(R"(
        QLabel {
            background: rgba(15,15,30,0.92);
            color: #FFD700;
            border: 1px solid rgba(255,215,0,0.5);
            border-radius: 10px;
            padding: 12px 24px;
            font-size: 14px;
            font-weight: bold;
        }
    )");
    m_toastLabel->setAlignment(Qt::AlignCenter);
    m_toastLabel->hide();

    auto* effect = new QGraphicsOpacityEffect(m_toastLabel);
    effect->setOpacity(0.0);
    m_toastLabel->setGraphicsEffect(effect);

    m_toastAnim = new QPropertyAnimation(this, "toastOpacity", this);
    m_toastTimer = new QTimer(this);
    m_toastTimer->setSingleShot(true);

    // 质量脉冲动画
    m_pulseAnim = new QPropertyAnimation(this, "massPulse", this);
    m_pulseAnim->setDuration(150);
    m_pulseAnim->setStartValue(0.0f);
    m_pulseAnim->setEndValue(0.15f);
    connect(m_pulseAnim, &QPropertyAnimation::finished, this, [this]() {
        QPropertyAnimation* decay = new QPropertyAnimation(this, "massPulse", this);
        decay->setDuration(150);
        decay->setStartValue(0.15f);
        decay->setEndValue(0.0f);
        decay->start(QAbstractAnimation::DeleteWhenStopped);
    });
}

void HUDOverlay::createTopBar() {
    QWidget* p = parentWidget();
    if (!p) return;

    m_topBar = new QWidget(p);
    m_topBar->setFixedHeight(48);
    m_topBar->setStyleSheet(
        "QWidget#hudTopBar { "
        "  background: rgba(10, 15, 30, 0.78);"
        "  border-bottom: 1px solid rgba(255,255,255,0.08);"
        "}"
    );
    m_topBar->setObjectName("hudTopBar");
    m_topBar->setAutoFillBackground(false);
    m_topBar->setAttribute(Qt::WA_StyledBackground, true);
    m_topBar->hide();

    auto* layout = new QHBoxLayout(m_topBar);
    layout->setContentsMargins(18, 6, 14, 6);
    layout->setSpacing(10);

    // 返回按钮
    m_backBtn = new QPushButton("←", m_topBar);
    m_backBtn->setFixedSize(34, 34);
    Style::applyIconButton(m_backBtn);
    layout->addWidget(m_backBtn);

    // 质量显示
    m_massLabel = new QLabel("10", m_topBar);
    m_massLabel->setMinimumWidth(90);
    m_massLabel->setStyleSheet(R"(
        QLabel {
            color: white;
            font-size: 24px;
            font-weight: bold;
            background: transparent;
            padding: 0;
        }
    )");
    layout->addWidget(m_massLabel);

    // 防护盾显示（默认隐藏）
    m_shieldLabel = new QLabel("", m_topBar);
    m_shieldLabel->setStyleSheet(R"(
        QLabel {
            color: #4FC3F7;
            font-size: 18px;
            font-weight: bold;
            background: transparent;
            padding: 0 8px;
        }
    )");
    m_shieldLabel->hide();
    layout->addWidget(m_shieldLabel);

    layout->addStretch();

    // 倒计时（居中）
    m_timerLabel = new QLabel("", m_topBar);
    m_timerLabel->setStyleSheet(R"(
        QLabel {
            color: white;
            font-size: 18px;
            font-weight: bold;
            background: transparent;
            padding: 0;
        }
    )");
    m_timerLabel->setAlignment(Qt::AlignCenter);
    layout->addWidget(m_timerLabel);

    layout->addStretch();

    QString btnStyle = R"(
        QPushButton {
            color: rgba(255,255,255,0.75);
            font-size: 13px;
            background: rgba(255,255,255,0.06);
            border: 1px solid rgba(255,255,255,0.1);
            padding: 4px 12px;
            border-radius: 6px;
            min-width: 60px;
        }
        QPushButton:hover { color: white; background: rgba(255,255,255,0.12); }
        QPushButton:checked { color: #FFD700; background: rgba(255,215,0,0.18); border-color: rgba(255,215,0,0.4); }
    )";

    // 排行榜按钮
    m_leaderboardBtn = new QPushButton("排行榜", m_topBar);
    m_leaderboardBtn->setFixedHeight(32);
    m_leaderboardBtn->setCursor(Qt::PointingHandCursor);
    m_leaderboardBtn->setCheckable(true);
    m_leaderboardBtn->setChecked(false);
    m_leaderboardBtn->setStyleSheet(btnStyle);
    layout->addWidget(m_leaderboardBtn);

    // 小地图按钮
    m_minimapBtn = new QPushButton("地图", m_topBar);
    m_minimapBtn->setFixedHeight(32);
    m_minimapBtn->setCursor(Qt::PointingHandCursor);
    m_minimapBtn->setCheckable(true);
    m_minimapBtn->setChecked(true);
    m_minimapBtn->setStyleSheet(btnStyle);
    layout->addWidget(m_minimapBtn);

    // 设置按钮
    m_settingsBtn = new QPushButton("⚙", m_topBar);
    m_settingsBtn->setFixedSize(34, 34);
    Style::applyIconButton(m_settingsBtn);
    layout->addWidget(m_settingsBtn);

    // 信号连接
    connect(m_backBtn, &QPushButton::clicked, this, &HUDOverlay::backToMenu);

    // 互斥切换：排行榜/地图同一时刻只能显示一个
    connect(m_leaderboardBtn, &QPushButton::toggled, this, [this](bool checked) {
        if (checked) {
            // 排行榜打开 → 关闭地图
            if (m_minimapBtn->isChecked()) {
                m_minimapBtn->blockSignals(true);
                m_minimapBtn->setChecked(false);
                m_minimapBtn->blockSignals(false);
                m_minimapVisible = false;
                emit minimapVisibilityChanged(false);
            }
            if (m_leaderboard) { m_leaderboard->show(); m_leaderboard->raise(); }
        } else {
            if (m_leaderboard) m_leaderboard->hide();
        }
    });

    connect(m_minimapBtn, &QPushButton::toggled, this, [this](bool checked) {
        m_minimapVisible = checked;
        emit minimapVisibilityChanged(checked);
        if (checked) {
            // 地图打开 → 关闭排行榜
            if (m_leaderboardBtn->isChecked()) {
                m_leaderboardBtn->blockSignals(true);
                m_leaderboardBtn->setChecked(false);
                m_leaderboardBtn->blockSignals(false);
                if (m_leaderboard) m_leaderboard->hide();
            }
        }
    });

    connect(m_settingsBtn, &QPushButton::clicked, this, [this]() {
        if (m_settingsPanel->isVisible()) {
            m_settingsPanel->hide();
        } else {
            // 设置面板作为顶级 Popup 窗口，使用全局坐标
            QPoint globalBtnPos = m_settingsBtn->mapToGlobal(QPoint(0, m_settingsBtn->height() + 4));
            int px = globalBtnPos.x() + m_settingsBtn->width() - m_settingsPanel->width();
            int py = globalBtnPos.y();
            m_settingsPanel->move(px, py);
            m_settingsPanel->show();
            m_settingsPanel->raise();
            m_settingsPanel->activateWindow();
        }
    });
}

void HUDOverlay::createSkillButtons() {
    QWidget* p = parentWidget();
    if (!p) return;

    m_skillPanel = new QWidget(p);
    m_skillPanel->setStyleSheet("background: transparent;");
    m_skillPanel->setAttribute(Qt::WA_TransparentForMouseEvents, false);
    m_skillPanel->hide();

    auto* layout = new QVBoxLayout(m_skillPanel);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(14);
    layout->setAlignment(Qt::AlignCenter);

    int btnSize = skillButtonSize();

    auto applySkillStyle = [](QPushButton* btn, int size, const QColor& accent) {
        btn->setFixedSize(size, size);
        btn->setStyleSheet(QString(R"(
            QPushButton {
                color: white;
                font-size: %1px;
                font-weight: bold;
                background: rgba(0,0,0,0.45);
                border: 2px solid %2;
                border-radius: %3px;
            }
            QPushButton:hover {
                background: rgba(255,255,255,0.15);
                border-color: white;
            }
            QPushButton:pressed {
                background: rgba(255,215,0,0.3);
            }
        )").arg(size / 4).arg(accent.name()).arg(size / 2));
    };

    // 分裂按钮
    m_splitBtn = new QPushButton(m_skillPanel);
    m_splitBtn->setText("分裂\nSpace");
    m_splitBtn->setCursor(Qt::PointingHandCursor);
    applySkillStyle(m_splitBtn, btnSize, Style::accentRed());
    layout->addWidget(m_splitBtn);

    // 吐球按钮
    m_ejectBtn = new QPushButton(m_skillPanel);
    m_ejectBtn->setText("吐球\nE");
    m_ejectBtn->setCursor(Qt::PointingHandCursor);
    applySkillStyle(m_ejectBtn, btnSize, Style::accentBlue());
    layout->addWidget(m_ejectBtn);

    // 转发点击给键盘事件 - 通过 GLWidget 触发
    connect(m_splitBtn, &QPushButton::clicked, this, [this]() {
        if (m_gameWidget) {
            QKeyEvent press(QEvent::KeyPress, Qt::Key_Space, Qt::NoModifier);
            QApplication::sendEvent(m_gameWidget, &press);
        }
    });
    connect(m_ejectBtn, &QPushButton::clicked, this, [this]() {
        if (m_gameWidget) {
            QKeyEvent press(QEvent::KeyPress, Qt::Key_E, Qt::NoModifier);
            QApplication::sendEvent(m_gameWidget, &press);
        }
    });
}

void HUDOverlay::createSettingsPanel() {
    // 设置面板使用 nullptr 作为父窗口，作为独立的 Popup 窗口
    // 这样可以避免影响 GLWidget 重绘和导致闪烁
    m_settingsPanel = new HUDSettingsPanel(nullptr);
    m_settingsPanel->hide();

    connect(m_settingsPanel, &HUDSettingsPanel::hudScaleChanged, this, [this](float) {
        updatePositions();
    });
    connect(m_settingsPanel, &HUDSettingsPanel::skillButtonSizeChanged, this, [this](int) {
        if (!m_splitBtn || !m_ejectBtn) return;
        int s = skillButtonSize();
        auto applyStyle = [s](QPushButton* btn, const QColor& accent) {
            btn->setFixedSize(s, s);
            btn->setStyleSheet(QString(R"(
                QPushButton {
                    color: white;
                    font-size: %1px;
                    font-weight: bold;
                    background: rgba(0,0,0,0.45);
                    border: 2px solid %2;
                    border-radius: %3px;
                }
                QPushButton:hover { background: rgba(255,255,255,0.15); border-color: white; }
                QPushButton:pressed { background: rgba(255,215,0,0.3); }
            )").arg(s / 4).arg(accent.name()).arg(s / 2));
        };
        applyStyle(m_splitBtn, Style::accentRed());
        applyStyle(m_ejectBtn, Style::accentBlue());
        updatePositions();
    });
    connect(m_settingsPanel, &HUDSettingsPanel::openFullSettings, this, &HUDOverlay::openSettings);
    connect(m_settingsPanel, &HUDSettingsPanel::quitGame, this, &HUDOverlay::quitGame);
}

void HUDOverlay::setLeaderboard(LeaderboardWidget* lb) {
    m_leaderboard = lb;
    if (m_leaderboard) {
        m_leaderboard->setParent(parentWidget());
        m_leaderboard->hide();
    }
}

void HUDOverlay::setMinimapVisible(bool v) {
    m_minimapVisible = v;
    if (m_minimapBtn) {
        m_minimapBtn->blockSignals(true);
        m_minimapBtn->setChecked(v);
        m_minimapBtn->blockSignals(false);
    }
    emit minimapVisibilityChanged(v);
}

void HUDOverlay::showHUD() {
    m_hudVisible = true;
    updatePositions();
    if (m_topBar) {
        m_topBar->show();
        m_topBar->raise();
    }
    if (m_skillPanel) {
        m_skillPanel->show();
        m_skillPanel->raise();
    }
    if (m_leaderboard && m_leaderboardBtn && m_leaderboardBtn->isChecked()) {
        m_leaderboard->show();
        m_leaderboard->raise();
    }
}

void HUDOverlay::hideHUD() {
    m_hudVisible = false;
    if (m_topBar) m_topBar->hide();
    if (m_skillPanel) m_skillPanel->hide();
    if (m_leaderboard) m_leaderboard->hide();
    if (m_settingsPanel) m_settingsPanel->hide();
    if (m_deathLabel) m_deathLabel->hide();
    if (m_toastLabel) m_toastLabel->hide();
    if (m_zoneWarningLabel) m_zoneWarningLabel->hide();
}

int HUDOverlay::skillButtonSize() const {
    switch (Config::instance().hudSkillButtonSize) {
    case 0: return 58;
    case 2: return 88;
    default: return 72;
    }
}

QColor HUDOverlay::massColor(float mass) const {
    if (mass < 100) return QColor(255, 255, 255);
    if (mass < 1000) return QColor(255, 215, 0);
    if (mass < 5000) return QColor(255, 140, 0);
    return QColor(255, 68, 68);
}

void HUDOverlay::updatePositions() {
    QWidget* pw = parentWidget();
    if (!pw) return;

    int w = pw->width();
    int h = pw->height();

    // 顶部横条 - 固定 48px 高
    if (m_topBar) {
        m_topBar->setGeometry(0, 0, w, 48);
        if (m_hudVisible) m_topBar->raise();
    }

    // 技能按钮 - 屏幕右下角
    if (m_skillPanel) {
        int btnSize = skillButtonSize();
        int panelW = btnSize;
        int panelH = btnSize * 2 + 14;
        int margin = 26;
        m_skillPanel->setGeometry(
            w - panelW - margin,
            h - panelH - margin,
            panelW,
            panelH
        );
        if (m_hudVisible) m_skillPanel->raise();
    }

    // 排行榜
    if (m_leaderboard && m_leaderboard->isVisible()) {
        m_leaderboard->move(w - 190, 60);
        m_leaderboard->raise();
    }
}

void HUDOverlay::resizeEvent(QResizeEvent*) {
    updatePositions();
}

void HUDOverlay::forwardMouseEvent(QMouseEvent* e) {
    if (m_gameWidget) {
        QApplication::sendEvent(m_gameWidget, e);
    }
}

void HUDOverlay::mouseMoveEvent(QMouseEvent* e) { forwardMouseEvent(e); }
void HUDOverlay::mousePressEvent(QMouseEvent* e) { forwardMouseEvent(e); }
void HUDOverlay::mouseReleaseEvent(QMouseEvent* e) { forwardMouseEvent(e); }
void HUDOverlay::wheelEvent(QWheelEvent* e) {
    if (m_gameWidget) QApplication::sendEvent(m_gameWidget, e);
}

void HUDOverlay::setMass(float mass) {
    if (!m_massLabel) return;
    int displayMass = static_cast<int>(mass);
    QColor c = massColor(mass);
    m_massLabel->setText(QString::number(displayMass));
    m_massLabel->setStyleSheet(QString(R"(
        QLabel {
            color: %1;
            font-size: 24px;
            font-weight: bold;
            background: transparent;
            padding: 0;
        }
    )").arg(c.name()));

    if (mass > m_prevMass + 0.5f) {
        m_pulseAnim->stop();
        m_pulseAnim->start();
    }
    m_prevMass = mass;
}

void HUDOverlay::setKills(int) {}
void HUDOverlay::setRank(int, int) {}
void HUDOverlay::setMode(const QString&) {}

void HUDOverlay::setShield(int count) {
    if (!m_shieldLabel) return;
    if (count <= 0) {
        m_shieldLabel->hide();
    } else {
        m_shieldLabel->setText(QString("🛡 %1").arg(count));
        m_shieldLabel->show();
    }
}

void HUDOverlay::setTimeRemaining(int seconds) {
    if (!m_timerLabel) return;
    if (seconds < 0) {
        m_timerLabel->hide();
        return;
    }
    m_timerLabel->show();
    int mins = seconds / 60;
    int secs = seconds % 60;
    m_timerLabel->setText(QString("⏱ %1:%2").arg(mins).arg(secs, 2, 10, QChar('0')));
    if (seconds < 60) {
        m_timerLabel->setStyleSheet("color: #e74c3c; font-size: 18px; font-weight: bold; background: transparent;");
    } else {
        m_timerLabel->setStyleSheet("color: white; font-size: 18px; font-weight: bold; background: transparent;");
    }
}

void HUDOverlay::setTeamScores(float, float) {}
void HUDOverlay::setZoneWarning(const QString& text) {
    if (!m_zoneWarningLabel) return;
    if (text.isEmpty()) {
        m_zoneWarningLabel->hide();
        return;
    }
    m_zoneWarningLabel->setText(text);
    m_zoneWarningLabel->show();
    m_zoneWarningLabel->adjustSize();
    QWidget* pw = parentWidget();
    if (pw) {
        m_zoneWarningLabel->move((pw->width() - m_zoneWarningLabel->width()) / 2, 62);
        m_zoneWarningLabel->raise();
    }
}

void HUDOverlay::showDeathOverlay(float respawnSeconds) {
    if (!m_deathLabel) return;
    m_deathLabel->setText(QString("💀 你被消灭了！\n%1 秒后复活...").arg(static_cast<int>(respawnSeconds) + 1));
    m_deathLabel->show();
    m_deathLabel->adjustSize();
    QWidget* pw = parentWidget();
    if (pw) {
        m_deathLabel->move((pw->width() - m_deathLabel->width()) / 2,
                           (pw->height() - m_deathLabel->height()) / 2);
        m_deathLabel->raise();
    }
}

void HUDOverlay::hideDeathOverlay() {
    if (m_deathLabel) m_deathLabel->hide();
}

void HUDOverlay::setToastOpacity(float v) {
    m_toastOpacity = v;
    if (!m_toastLabel) return;
    auto* effect = qobject_cast<QGraphicsOpacityEffect*>(m_toastLabel->graphicsEffect());
    if (effect) effect->setOpacity(v);
}

void HUDOverlay::setMassPulse(float v) {
    m_massPulse = v;
    if (!m_massLabel) return;
    float s = 1.0f + v;
    QFont f = m_massLabel->font();
    f.setPixelSize(static_cast<int>(24 * s));
    m_massLabel->setFont(f);
}

void HUDOverlay::showAchievement(const QString& name, const QString& description) {
    if (m_toastActive || !m_toastLabel) return;

    m_toastActive = true;
    m_toastLabel->setText(QString("🏆 解锁成就：%1\n%2").arg(name).arg(description));

    QWidget* pw = parentWidget();
    if (pw) {
        m_toastLabel->adjustSize();
        int tw = m_toastLabel->width();
        int th = m_toastLabel->height();
        m_toastLabel->setGeometry((pw->width() - tw) / 2, 70, tw, th);
    }
    m_toastLabel->show();
    m_toastLabel->raise();

    m_toastAnim->stop();
    m_toastAnim->setDuration(300);
    m_toastAnim->setStartValue(0.0);
    m_toastAnim->setEndValue(1.0);
    m_toastAnim->start();

    m_toastTimer->disconnect();
    connect(m_toastTimer, &QTimer::timeout, this, [this]() {
        m_toastAnim->stop();
        m_toastAnim->setDuration(300);
        m_toastAnim->setStartValue(1.0);
        m_toastAnim->setEndValue(0.0);
        m_toastAnim->start();
        connect(m_toastAnim, &QPropertyAnimation::finished, this, [this]() {
            m_toastLabel->hide();
            m_toastActive = false;
        });
    });
    m_toastTimer->start(3000);
}
