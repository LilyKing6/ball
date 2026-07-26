#include "MainMenu.h"
#include "Style.h"
#include "ranking/RankSystem.h"
#include "ranking/SeasonManager.h"
#include "storage/DatabaseManager.h"
#include "util/Random.h"
#include <QPainter>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QGraphicsDropShadowEffect>
#include <QDateTime>
#include <QSqlQuery>
#include <QtMath>
#include <QPainterPath>

MainMenu::MainMenu(QWidget* parent) : QWidget(parent) {
    setAutoFillBackground(true);

    initStars();
    initBalls();

    m_animTimer = new QTimer(this);
    connect(m_animTimer, &QTimer::timeout, this, [this]() {
        float dt = 0.033f;
        m_time += dt;
        m_pulsePhase += dt * 2.5f;
        if (m_pulsePhase > 3.14159f * 2) m_pulsePhase -= 3.14159f * 2;
        updateBalls();

        // 脉冲动画：调整阴影大小
        if (m_startBtn) {
            auto* shadow = qobject_cast<QGraphicsDropShadowEffect*>(m_startBtn->graphicsEffect());
            if (shadow) {
                float pulse = 1.0f + 0.15f * qSin(m_pulsePhase);
                shadow->setBlurRadius(24 * pulse);
            }
        }
        update();
    });
    m_animTimer->start(33);
}

void MainMenu::refreshLayout() {
    if (m_layoutInitialized) return;
    m_layoutInitialized = true;

    setupRightPanel();
    setupLeftPanel();
    refreshRank();
}

void MainMenu::setupLeftPanel() {
    m_leftPanel = new QWidget(this);
    m_leftPanel->setAttribute(Qt::WA_TranslucentBackground);
    m_leftPanel->setFixedSize(300, 460);
    m_leftPanel->setStyleSheet(Style::glassPanelStyle());

    auto* shadow = new QGraphicsDropShadowEffect(m_leftPanel);
    shadow->setBlurRadius(40);
    shadow->setColor(QColor(0, 0, 0, 180));
    shadow->setOffset(0, 12);
    m_leftPanel->setGraphicsEffect(shadow);

    auto* layout = new QVBoxLayout(m_leftPanel);
    layout->setContentsMargins(28, 32, 28, 28);
    layout->setSpacing(16);
    layout->setAlignment(Qt::AlignTop);

    // 头像 - 显示昵称首字
    m_avatarLabel = new QLabel(m_leftPanel);
    m_avatarLabel->setFixedSize(96, 96);
    m_avatarLabel->setAlignment(Qt::AlignCenter);
    m_avatarLabel->setStyleSheet(R"(
        QLabel {
            background: qradialgradient(cx:0.4,cy:0.35,radius:0.7,
                stop:0 rgba(255,215,0,0.35),
                stop:0.6 rgba(255,180,0,0.15),
                stop:1 rgba(255,215,0,0.0));
            border: 2px solid rgba(255,215,0,0.4);
            border-radius: 48px;
            font-size: 42px;
            font-weight: bold;
            color: #FFE066;
        }
    )");
    layout->addWidget(m_avatarLabel, 0, Qt::AlignCenter);

    // 昵称输入
    m_nameInput = new QLineEdit(m_leftPanel);
    m_nameInput->setPlaceholderText("输入你的昵称...");
    m_nameInput->setText("Player");
    m_nameInput->setMaxLength(12);
    m_nameInput->setFixedHeight(44);
    m_nameInput->setAlignment(Qt::AlignCenter);
    m_nameInput->setStyleSheet(Style::lineEditStyle());
    layout->addWidget(m_nameInput);

    // 分隔线
    auto* separator = new QFrame(m_leftPanel);
    separator->setFixedHeight(1);
    separator->setStyleSheet("background: rgba(255,255,255,0.1); border: none;");
    layout->addSpacing(4);
    layout->addWidget(separator);
    layout->addSpacing(4);

    // 段位标签
    m_tierLabel = new QLabel("青铜 III", m_leftPanel);
    m_tierLabel->setStyleSheet(Style::labelStyle(Style::accentGold(), 26, true));
    m_tierLabel->setAlignment(Qt::AlignCenter);
    layout->addWidget(m_tierLabel);

    // ELO 标签
    m_eloLabel = new QLabel("ELO: 1000", m_leftPanel);
    m_eloLabel->setStyleSheet(Style::labelStyle(Style::textSecondary(), 14, false));
    m_eloLabel->setAlignment(Qt::AlignCenter);
    layout->addWidget(m_eloLabel);

    // ELO 进度条
    m_eloProgress = new QProgressBar(m_leftPanel);
    m_eloProgress->setRange(0, 1000);
    m_eloProgress->setValue(250);
    m_eloProgress->setTextVisible(false);
    m_eloProgress->setFixedHeight(14);
    m_eloProgress->setStyleSheet(
        "QProgressBar { background: rgba(255,255,255,0.08); border: none; border-radius: 7px; }"
        "QProgressBar::chunk { background: qlineargradient(x1:0,y1:0,x2:1,y2:0,stop:0 #CD7F32,stop:1 #FFD700); border-radius: 7px; }");
    layout->addWidget(m_eloProgress);

    // 进度文字
    m_progressText = new QLabel("距白银还需 200 ELO", m_leftPanel);
    m_progressText->setStyleSheet(Style::labelStyle(Style::textMuted(), 12, false));
    m_progressText->setAlignment(Qt::AlignCenter);
    layout->addWidget(m_progressText);

    layout->addSpacing(8);

    // 赛季信息
    m_seasonLabel = new QLabel("S1 赛季 · 剩余 28 天", m_leftPanel);
    m_seasonLabel->setStyleSheet(Style::labelStyle(Style::textMuted(), 12, false));
    m_seasonLabel->setAlignment(Qt::AlignCenter);
    layout->addWidget(m_seasonLabel);

    layout->addStretch();
}

void MainMenu::setupRightPanel() {
    m_rightPanel = new QWidget(this);
    m_rightPanel->setAttribute(Qt::WA_TranslucentBackground);
    m_rightPanel->setFixedSize(380, 520);
    m_rightPanel->setStyleSheet(Style::glassPanelStyle());

    auto* shadow = new QGraphicsDropShadowEffect(m_rightPanel);
    shadow->setBlurRadius(40);
    shadow->setColor(QColor(0, 0, 0, 180));
    shadow->setOffset(0, 12);
    m_rightPanel->setGraphicsEffect(shadow);

    auto* layout = new QVBoxLayout(m_rightPanel);
    layout->setContentsMargins(30, 36, 30, 30);
    layout->setSpacing(14);
    layout->setAlignment(Qt::AlignVCenter);

    // 标题区域占位
    m_titleLabel = new QLabel("", m_rightPanel);
    m_titleLabel->setFixedHeight(84);
    m_titleLabel->setAlignment(Qt::AlignCenter);
    layout->addWidget(m_titleLabel);

    // 口号
    m_subtitleLabel = new QLabel("大鱼吃小鱼，吞并天下", m_rightPanel);
    m_subtitleLabel->setStyleSheet(
        "font-size: 14px; color: #5a5a7a; background: transparent; letter-spacing: 3px;");
    m_subtitleLabel->setAlignment(Qt::AlignCenter);
    layout->addWidget(m_subtitleLabel);

    layout->addSpacing(20);

    // 开始游戏按钮 - 更大更显眼
    m_startBtn = new QPushButton("开始游戏", m_rightPanel);
    m_startBtn->setFixedSize(320, 68);
    m_startBtn->setCursor(Qt::PointingHandCursor);
    m_startBtn->setStyleSheet(R"(
        QPushButton {
            font-size: 22px;
            font-weight: bold;
            color: white;
            background: qlineargradient(x1:0,y1:0,x2:1,y2:0,
                stop:0 #FF6B6B, stop:0.3 #FF8E53, stop:0.6 #FFA500, stop:1 #FFD700);
            border: none;
            border-radius: 34px;
        }
        QPushButton:hover {
            background: qlineargradient(x1:0,y1:0,x2:1,y2:0,
                stop:0 #FF8585, stop:0.3 #FFA873, stop:0.6 #FFB733, stop:1 #FFE066);
        }
        QPushButton:pressed {
            background: qlineargradient(x1:0,y1:0,x2:1,y2:0,
                stop:0 #E05555, stop:0.3 #E67E4A, stop:0.6 #E69500, stop:1 #E6C200);
        }
    )");

    auto* btnShadow = new QGraphicsDropShadowEffect(m_startBtn);
    btnShadow->setBlurRadius(28);
    btnShadow->setColor(QColor(255, 140, 50, 160));
    btnShadow->setOffset(0, 8);
    m_startBtn->setGraphicsEffect(btnShadow);

    layout->addWidget(m_startBtn, 0, Qt::AlignCenter);

    layout->addSpacing(24);

    // 次要按钮网格 (2x2) - 纯文字去掉 emoji
    auto* btnGrid = new QGridLayout();
    btnGrid->setSpacing(12);
    btnGrid->setContentsMargins(0, 0, 0, 0);

    struct BtnInfo { QString text; QPushButton** ptr; };
    QList<BtnInfo> btns = {
        {"设置", &m_settingsBtn},
        {"游戏记录", &m_recordsBtn},
        {"段位", &m_rankBtn},
        {"成就", &m_achievementBtn},
    };

    for (int i = 0; i < btns.size(); i++) {
        auto* btn = new QPushButton(btns[i].text, m_rightPanel);
        *btns[i].ptr = btn;
        btn->setFixedSize(152, 44);
        btn->setCursor(Qt::PointingHandCursor);
        btn->setStyleSheet(R"(
            QPushButton {
                font-size: 15px;
                color: #b0b0c0;
                background: rgba(255,255,255,0.05);
                border: 1px solid rgba(255,255,255,0.10);
                border-radius: 12px;
            }
            QPushButton:hover {
                color: white;
                background: rgba(255,255,255,0.10);
                border-color: rgba(255,255,255,0.22);
            }
            QPushButton:pressed {
                background: rgba(255,255,255,0.04);
            }
        )");
        btnGrid->addWidget(btn, i / 2, i % 2);
    }

    layout->addLayout(btnGrid);
    layout->addStretch();

    connect(m_startBtn, &QPushButton::clicked, this, &MainMenu::showModeSelect);
    connect(m_settingsBtn, &QPushButton::clicked, this, &MainMenu::openSettings);
    connect(m_recordsBtn, &QPushButton::clicked, this, &MainMenu::showRecords);
    connect(m_rankBtn, &QPushButton::clicked, this, &MainMenu::showRank);
    connect(m_achievementBtn, &QPushButton::clicked, this, &MainMenu::showAchievements);
}

void MainMenu::initStars() {
    m_stars.clear();
    int count = 150;
    for (int i = 0; i < count; i++) {
        Star s;
        s.pos = {randFloat(0, 1), randFloat(0, 1)};
        s.size = randFloat(0.8f, 2.8f);
        s.baseAlpha = randFloat(0.2f, 0.9f);
        s.phase = randFloat(0, 3.14159f * 2);
        s.speed = randFloat(0.3f, 2.5f);

        int colorType = randInt(0, 14);
        if (colorType < 8)       s.color = QColor(255, 255, 255);
        else if (colorType < 10) s.color = QColor(160, 200, 255);
        else if (colorType < 12) s.color = QColor(255, 240, 180);
        else if (colorType < 13) s.color = QColor(200, 160, 255);
        else                     s.color = QColor(160, 255, 200);

        m_stars.append(s);
    }
}

void MainMenu::initBalls() {
    m_balls.clear();
    for (int i = 0; i < 50; i++) {
        MenuBall b;
        b.pos = {randFloat(0, 1), randFloat(0, 1)};
        b.vel = {randFloat(-40, 40), randFloat(-40, 40)};
        b.radius = randFloat(15, 90);
        b.color = QColor::fromHsv(randInt(0, 359), 120 + randInt(0, 100), 170 + randInt(0, 85));
        b.alpha = randFloat(0.03f, 0.12f);
        m_balls.append(b);
    }
}

void MainMenu::updateBalls() {
    float w = width(), h = height();
    if (w <= 0 || h <= 0) return;
    for (auto& b : m_balls) {
        b.pos += b.vel * 0.016f;
        if (b.pos.x() < -b.radius) b.pos.setX(w + b.radius);
        if (b.pos.x() > w + b.radius) b.pos.setX(-b.radius);
        if (b.pos.y() < -b.radius) b.pos.setY(h + b.radius);
        if (b.pos.y() > h + b.radius) b.pos.setY(-b.radius);
    }
}

void MainMenu::drawBackground(QPainter& p) {
    p.fillRect(rect(), Style::bgDark());
}

void MainMenu::drawStars(QPainter& p) {
    float w = width(), h = height();
    for (const auto& s : m_stars) {
        float alpha = s.baseAlpha * (0.5f + 0.5f * sinf(m_time * s.speed + s.phase));
        QColor c = s.color;
        c.setAlphaF(alpha);
        QPointF pos(s.pos.x() * w, s.pos.y() * h);

        p.setPen(Qt::NoPen);
        p.setBrush(c);
        p.drawEllipse(pos, s.size, s.size);

        // 较大星点加微弱光晕
        if (s.size > 1.8f) {
            c.setAlphaF(alpha * 0.25f);
            p.setBrush(c);
            p.drawEllipse(pos, s.size * 2.5f, s.size * 2.5f);
        }
    }
}

void MainMenu::drawTitle(QPainter& p) {
    if (!m_rightPanel) return;

    QRect rightRect = m_rightPanel->geometry();
    QRect titleRect(rightRect.x(), rightRect.y() + 30, rightRect.width(), 84);

    QList<QColor> colors;
    colors << QColor("#4fc3f7") << QColor("#ab47bc") << QColor("#f48fb1") << QColor("#ffcc80");
    Style::drawGradientTitle(p, "球球大作战", titleRect, colors, 56);
}

void MainMenu::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.setRenderHint(QPainter::TextAntialiasing);

    drawBackground(p);

    // 漂浮球 - 不同透明度的远景装饰
    float w = width(), h = height();
    for (auto& b : m_balls) {
        QColor c = b.color;
        c.setAlphaF(b.alpha);
        p.setBrush(c);
        p.setPen(Qt::NoPen);
        p.drawEllipse(QPointF(b.pos.x() * w, b.pos.y() * h), b.radius, b.radius);
    }

    // 星点
    drawStars(p);

    // 暗角
    Style::drawVignette(p, rect(), 0.55f);

    // 标题
    drawTitle(p);
}

void MainMenu::resizeEvent(QResizeEvent*) {
    float w = width(), h = height();
    initStars();

    // 重新初始化球的位置到屏幕坐标系
    for (auto& b : m_balls) {
        b.pos = {randFloat(0, 1), randFloat(0, 1)};
    }

    // 左右面板居中定位
    if (m_leftPanel && m_rightPanel) {
        int panelGap = 50;
        int totalWidth = m_leftPanel->width() + panelGap + m_rightPanel->width();
        int startX = (width() - totalWidth) / 2;
        int leftY = (height() - m_leftPanel->height()) / 2;
        int rightY = (height() - m_rightPanel->height()) / 2;

        m_leftPanel->move(startX, leftY);
        m_rightPanel->move(startX + m_leftPanel->width() + panelGap, rightY);
    }
}

void MainMenu::refreshRank() {
    auto& db = DatabaseManager::instance().database();
    QSqlQuery q(db);
    q.prepare("SELECT elo, rank_tier FROM player_profile WHERE player_id = ?");
    q.addBindValue("local");

    int elo = 1000;
    if (q.exec() && q.next()) {
        elo = q.value("elo").toInt();
    }

    TierInfo info = RankSystem::tierInfoForElo(elo);
    float progress = RankSystem::progressToNextTier(elo);
    RankTier currentTier = RankSystem::tierFromElo(elo);

    if (m_tierLabel) {
        m_tierLabel->setText(info.name);
        m_tierLabel->setStyleSheet(Style::labelStyle(info.color, 26, true));
    }

    if (m_eloLabel) {
        m_eloLabel->setText(QString("ELO: %1").arg(elo));
    }

    if (m_eloProgress) {
        m_eloProgress->setValue(static_cast<int>(progress * 1000));
        QString nextColor = (currentTier == RankTier::King_I)
            ? info.color.name()
            : RankSystem::tierInfo(static_cast<RankTier>(static_cast<int>(currentTier) + 1)).color.name();
        m_eloProgress->setStyleSheet(QString(
            "QProgressBar { background: rgba(255,255,255,0.08); border: none; border-radius: 7px; }"
            "QProgressBar::chunk { background: qlineargradient(x1:0,y1:0,x2:1,y2:0,stop:0 %1,stop:1 %2); border-radius: 7px; }"
        ).arg(info.color.name()).arg(nextColor));
    }

    if (m_progressText) {
        if (currentTier == RankTier::King_I) {
            m_progressText->setText("已达最高段位");
        } else {
            TierInfo nextInfo = RankSystem::tierInfo(static_cast<RankTier>(static_cast<int>(currentTier) + 1));
            int needed = nextInfo.minElo - elo;
            m_progressText->setText(QString("距%1还需 %2 ELO").arg(nextInfo.name).arg(needed));
        }
    }

    if (m_seasonLabel) {
        auto& sm = SeasonManager::instance();
        m_seasonLabel->setText(QString("%1 赛季 · 剩余 %2 天")
            .arg(sm.currentSeasonId()).arg(sm.daysRemaining()));
    }
}
