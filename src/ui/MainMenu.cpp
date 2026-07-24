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

    // 初始化动画数据
    initStars();
    initBalls();

    // 动画定时器 (30 FPS)
    m_animTimer = new QTimer(this);
    connect(m_animTimer, &QTimer::timeout, this, [this]() {
        m_time += 0.033f;
        updateStars(0.033f);
        updateMeteors(0.033f);
        updateBalls();
        update();
    });
    m_animTimer->start(33);

    // 流星生成定时器
    m_meteorTimer = new QTimer(this);
    connect(m_meteorTimer, &QTimer::timeout, this, [this]() {
        if (randFloat(0, 1) < 0.3f) { // 30% 概率生成
            initMeteor();
        }
        m_meteorTimer->setInterval(3000 + randInt(0, 5000)); // 3-8秒
    });
    m_meteorTimer->start(4000);
}

void MainMenu::refreshLayout() {
    if (m_layoutInitialized) return;
    m_layoutInitialized = true;

    // 创建左右面板
    setupLeftPanel();
    setupRightPanel();

    // 初始刷新数据
    refreshRank();
}

void MainMenu::setupLeftPanel() {
    m_leftPanel = new QWidget(this);
    m_leftPanel->setAttribute(Qt::WA_TranslucentBackground);
    m_leftPanel->setFixedWidth(280);
    m_leftPanel->setStyleSheet(Style::glassPanelStyle());

    auto* shadow = new QGraphicsDropShadowEffect(m_leftPanel);
    shadow->setBlurRadius(30);
    shadow->setColor(QColor(0, 0, 0, 160));
    shadow->setOffset(0, 8);
    m_leftPanel->setGraphicsEffect(shadow);

    auto* layout = new QVBoxLayout(m_leftPanel);
    layout->setContentsMargins(24, 28, 24, 28);
    layout->setSpacing(14);
    layout->setAlignment(Qt::AlignTop);

    // 头像占位
    m_avatarLabel = new QLabel(m_leftPanel);
    m_avatarLabel->setFixedSize(88, 88);
    m_avatarLabel->setAlignment(Qt::AlignCenter);
    m_avatarLabel->setText("★");
    m_avatarLabel->setStyleSheet(R"(
        QLabel {
            background: qradialgradient(cx:0.5,cy:0.5,radius:0.8,
                stop:0 rgba(255,215,0,0.25),
                stop:0.5 rgba(255,215,0,0.08),
                stop:1 rgba(255,215,0,0.0));
            border: 2px solid rgba(255,215,0,0.35);
            border-radius: 44px;
            font-size: 36px;
            color: #FFD700;
        }
    )");
    layout->addWidget(m_avatarLabel, 0, Qt::AlignCenter);

    // 昵称输入
    m_nameInput = new QLineEdit(m_leftPanel);
    m_nameInput->setPlaceholderText("输入你的昵称...");
    m_nameInput->setText("Player");
    m_nameInput->setMaxLength(12);
    m_nameInput->setFixedHeight(42);
    m_nameInput->setAlignment(Qt::AlignCenter);
    m_nameInput->setStyleSheet(Style::lineEditStyle());
    layout->addWidget(m_nameInput);

    // 分隔线
    auto* separator = new QFrame(m_leftPanel);
    separator->setFixedHeight(1);
    separator->setStyleSheet("background: rgba(255,255,255,0.1);");
    layout->addSpacing(8);
    layout->addWidget(separator);
    layout->addSpacing(8);

    // 段位标签
    m_tierLabel = new QLabel("青铜 III", m_leftPanel);
    m_tierLabel->setStyleSheet(Style::labelStyle(Style::accentGold(), 24, true));
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
    m_eloProgress->setFixedHeight(12);
    m_eloProgress->setStyleSheet(Style::sliderStyle(Style::accentGold()) + "QProgressBar { background: rgba(255,255,255,0.1); border-radius: 6px; } QProgressBar::chunk { border-radius: 6px; background: qlineargradient(x1:0,y1:0,x2:1,y2:0,stop:0 #CD7F32,stop:1 #FFD700); }");
    layout->addWidget(m_eloProgress);

    // 进度文字
    m_progressText = new QLabel("距白银还需 200 ELO", m_leftPanel);
    m_progressText->setStyleSheet(Style::labelStyle(Style::textMuted(), 12, false));
    m_progressText->setAlignment(Qt::AlignCenter);
    layout->addWidget(m_progressText);

    layout->addSpacing(12);

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
    m_rightPanel->setFixedWidth(340);
    m_rightPanel->setStyleSheet(Style::glassPanelStyle());

    auto* shadow = new QGraphicsDropShadowEffect(m_rightPanel);
    shadow->setBlurRadius(30);
    shadow->setColor(QColor(0, 0, 0, 160));
    shadow->setOffset(0, 8);
    m_rightPanel->setGraphicsEffect(shadow);

    auto* layout = new QVBoxLayout(m_rightPanel);
    layout->setContentsMargins(24, 28, 24, 28);
    layout->setSpacing(16);
    layout->setAlignment(Qt::AlignVCenter);

    // 标题区域占位
    m_titleLabel = new QLabel("", m_rightPanel);
    m_titleLabel->setFixedHeight(70);
    m_titleLabel->setAlignment(Qt::AlignCenter);
    layout->addWidget(m_titleLabel);

    // 副标题
    m_subtitleLabel = new QLabel("AGAR.IO CLONE", m_rightPanel);
    m_subtitleLabel->setStyleSheet("font-size: 13px; color: #666; background: transparent; letter-spacing: 2px;");
    m_subtitleLabel->setAlignment(Qt::AlignCenter);
    layout->addWidget(m_subtitleLabel);

    layout->addSpacing(16);

    // 开始游戏按钮
    m_startBtn = new QPushButton("开始游戏", m_rightPanel);
    m_startBtn->setFixedSize(300, 60);
    Style::applyPrimaryButton(m_startBtn);
    m_startBtn->setStyleSheet(Style::primaryButtonStyle());

    // 按钮悬停动画效果
    auto* btnShadow = new QGraphicsDropShadowEffect(m_startBtn);
    btnShadow->setBlurRadius(24);
    btnShadow->setColor(QColor(231, 76, 60, 140));
    btnShadow->setOffset(0, 6);
    m_startBtn->setGraphicsEffect(btnShadow);

    layout->addWidget(m_startBtn, 0, Qt::AlignCenter);

    layout->addSpacing(20);

    // 次要按钮网格 (2x2)
    auto* btnGrid = new QGridLayout();
    btnGrid->setSpacing(14);
    btnGrid->setContentsMargins(0, 0, 0, 0);

    m_settingsBtn = new QPushButton("⚙ 设置", m_rightPanel);
    m_recordsBtn = new QPushButton("📋 记录", m_rightPanel);
    m_rankBtn = new QPushButton("🏆 段位", m_rightPanel);
    m_achievementBtn = new QPushButton("⭐ 成就", m_rightPanel);

    m_settingsBtn->setFixedSize(140, 40);
    m_recordsBtn->setFixedSize(140, 40);
    m_rankBtn->setFixedSize(140, 40);
    m_achievementBtn->setFixedSize(140, 40);

    Style::applySecondaryButton(m_settingsBtn);
    Style::applySecondaryButton(m_recordsBtn);
    Style::applySecondaryButton(m_rankBtn);
    Style::applySecondaryButton(m_achievementBtn);

    btnGrid->addWidget(m_settingsBtn, 0, 0);
    btnGrid->addWidget(m_recordsBtn, 0, 1);
    btnGrid->addWidget(m_rankBtn, 1, 0);
    btnGrid->addWidget(m_achievementBtn, 1, 1);

    layout->addLayout(btnGrid);
    layout->addStretch();

    // 连接信号
    connect(m_startBtn, &QPushButton::clicked, this, &MainMenu::showModeSelect);
    connect(m_settingsBtn, &QPushButton::clicked, this, &MainMenu::openSettings);
    connect(m_recordsBtn, &QPushButton::clicked, this, &MainMenu::showRecords);
    connect(m_rankBtn, &QPushButton::clicked, this, &MainMenu::showRank);
    connect(m_achievementBtn, &QPushButton::clicked, this, &MainMenu::showAchievements);
}

void MainMenu::initStars() {
    m_stars.clear();
    int count = 120;
    for (int i = 0; i < count; i++) {
        Star s;
        s.pos = {randFloat(0, 1), randFloat(0, 1)}; // 相对位置 0-1
        s.size = randFloat(1.0f, 3.0f);
        s.baseAlpha = randFloat(0.3f, 0.9f);
        s.phase = randFloat(0, 3.14159f * 2);
        s.speed = randFloat(0.5f, 2.0f);

        // 80% 白色，10% 淡蓝，10% 淡黄
        int colorType = randInt(0, 9);
        if (colorType < 8) s.color = QColor(255, 255, 255);
        else if (colorType < 9) s.color = QColor(136, 204, 255);
        else s.color = QColor(255, 238, 170);

        m_stars.append(s);
    }
}

void MainMenu::initBalls() {
    m_balls.clear();
    for (int i = 0; i < 20; i++) {
        MenuBall b;
        b.pos = {randFloat(0, width()), randFloat(0, height())};
        b.vel = {randFloat(-30, 30), randFloat(-30, 30)};
        b.radius = randFloat(20, 80);
        b.color = QColor::fromHsv(randInt(0, 359), 150 + randInt(0, 80), 180 + randInt(0, 75));
        m_balls.append(b);
    }
}

void MainMenu::initMeteor() {
    Meteor m;
    float startX = randFloat(0.6f, 1.0f) * width();
    float startY = randFloat(0.0f, 0.3f) * height();
    float angle = randFloat(200, 240) * 3.14159f / 180.0f; // 200-240度方向
    float distance = randFloat(300, 600);

    m.start = QPointF(startX, startY);
    m.end = QPointF(startX + cosf(angle) * distance, startY + sinf(angle) * distance);
    m.current = m.start;
    m.progress = 0.0f;
    m.length = randFloat(50, 100);
    m.duration = randFloat(0.5f, 1.0f);
    m.active = true;

    m_meteors.append(m);
}

void MainMenu::updateStars(float) {
    // 星点闪烁由 paintEvent 根据 m_time 计算
}

void MainMenu::updateMeteors(float dt) {
    for (auto& m : m_meteors) {
        if (!m.active) continue;
        m.progress += dt / m.duration;
        if (m.progress >= 1.0f) {
            m.active = false;
        } else {
            m.current = QPointF(
                m.start.x() + (m.end.x() - m.start.x()) * m.progress,
                m.start.y() + (m.end.y() - m.start.y()) * m.progress
            );
        }
    }
    // 清理已完成的流星
    m_meteors.erase(std::remove_if(m_meteors.begin(), m_meteors.end(),
        [](const Meteor& m) { return !m.active; }), m_meteors.end());
}

void MainMenu::updateBalls() {
    for (auto& b : m_balls) {
        b.pos += b.vel * 0.016f;
        if (b.pos.x() < -b.radius) b.pos.setX(width() + b.radius);
        if (b.pos.x() > width() + b.radius) b.pos.setX(-b.radius);
        if (b.pos.y() < -b.radius) b.pos.setY(height() + b.radius);
        if (b.pos.y() > height() + b.radius) b.pos.setY(-b.radius);
    }
}

void MainMenu::drawStars(QPainter& p) {
    for (const auto& s : m_stars) {
        float alpha = s.baseAlpha * (0.5f + 0.5f * sinf(m_time * s.speed + s.phase));
        QColor c = s.color;
        c.setAlphaF(alpha);

        QPointF pos(s.pos.x() * width(), s.pos.y() * height());

        p.setPen(Qt::NoPen);
        p.setBrush(c);
        p.drawEllipse(pos, s.size, s.size);

        // 大星点加光晕
        if (s.size > 2.0f) {
            c.setAlphaF(alpha * 0.3f);
            p.setBrush(c);
            p.drawEllipse(pos, s.size * 2, s.size * 2);
        }
    }
}

void MainMenu::drawMeteors(QPainter& p) {
    for (const auto& m : m_meteors) {
        if (!m.active) continue;

        // 计算拖尾方向
        QPointF dir = m.end - m.start;
        float len = sqrtf(dir.x()*dir.x() + dir.y()*dir.y());
        if (len < 1) continue;
        dir /= len;

        // 拖尾渐变
        float tailLen = m.length;
        QPointF tailStart = m.current;
        QPointF tailEnd = m.current - dir * tailLen;

        QLinearGradient grad(tailEnd, tailStart);
        grad.setColorAt(0, QColor(255, 255, 255, 0));
        grad.setColorAt(0.7, QColor(255, 255, 255, 100));
        grad.setColorAt(1, QColor(255, 255, 255, 200));

        p.setPen(QPen(grad, 2));
        p.drawLine(tailEnd, tailStart);

        // 头部光点
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(255, 255, 255, 230));
        p.drawEllipse(m.current, 2, 2);
    }
}

void MainMenu::drawTitle(QPainter& p) {
    if (!m_rightPanel) return;

    QRect rightRect = m_rightPanel->geometry();
    QRect titleRect(rightRect.x(), rightRect.y() + 24, rightRect.width(), 70);

    QList<QColor> colors;
    colors << QColor("#4fc3f7") << QColor("#ba68c8") << QColor("#f48fb1");
    Style::drawGradientTitle(p, "球球大作战", titleRect, colors, 44);
}

void MainMenu::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.setRenderHint(QPainter::TextAntialiasing);

    // 深色背景
    p.fillRect(rect(), Style::bgDark());

    // 绘制星点
    drawStars(p);

    // 绘制流星
    drawMeteors(p);

    // 绘制远景漂浮球（低透明度）
    for (auto& b : m_balls) {
        QColor c = b.color;
        c.setAlpha(12);
        p.setBrush(c);
        p.setPen(Qt::NoPen);
        p.drawEllipse(b.pos, b.radius, b.radius);
    }

    // 绘制渐变描边标题
    drawTitle(p);
}

void MainMenu::resizeEvent(QResizeEvent*) {
    initStars(); // 重新分布星点
    initBalls();

    // 更新左右面板位置，整体居中
    if (m_leftPanel && m_rightPanel) {
        int panelGap = 40;
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
    QString tierName = "青铜 III";
    if (q.exec() && q.next()) {
        elo = q.value("elo").toInt();
        tierName = q.value("rank_tier").toString();
    }

    TierInfo info = RankSystem::tierInfoForElo(elo);
    float progress = RankSystem::progressToNextTier(elo);
    RankTier currentTier = RankSystem::tierFromElo(elo);

    if (m_tierLabel) {
        m_tierLabel->setText(info.name);
        m_tierLabel->setStyleSheet(Style::labelStyle(info.color, 24, true));
    }

    if (m_eloLabel) {
        m_eloLabel->setText(QString("ELO: %1").arg(elo));
    }

    if (m_eloProgress) {
        m_eloProgress->setValue(static_cast<int>(progress * 1000));

        // 更新进度条颜色
        QString nextColor = (currentTier == RankTier::King_I) ? info.color.name() :
            RankSystem::tierInfo(static_cast<RankTier>(static_cast<int>(currentTier) + 1)).color.name();
        m_eloProgress->setStyleSheet(QString(R"(
            QProgressBar { background: rgba(255,255,255,0.1); border-radius: 6px; border: none; }
            QProgressBar::chunk { background: qlineargradient(x1:0,y1:0,x2:1,y2:0,stop:0 %1,stop:1 %2); border-radius: 6px; }
        )").arg(info.color.name()).arg(nextColor));
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
