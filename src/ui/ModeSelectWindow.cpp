#include "ModeSelectWindow.h"
#include "Style.h"
#include "util/Config.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QInputDialog>
#include <QLineEdit>
#include <QIntValidator>
#include <QGraphicsDropShadowEffect>
#include <QScrollArea>
#include <QEvent>
#include <QMouseEvent>

ModeSelectWindow::ModeSelectWindow(QWidget* parent)
    : SubWindow("选择模式", parent, 520, 560) {

    auto* l = contentLayout();
    l->setSpacing(0);
    l->setContentsMargins(0, 0, 0, 0);

    auto* scroll = new QScrollArea(this);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scroll->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    scroll->setStyleSheet(R"(
        QScrollArea { background: transparent; border: none; }
        QScrollBar:vertical {
            background: rgba(0,0,0,0.2); width: 8px; border-radius: 4px;
        }
        QScrollBar::handle:vertical {
            background: rgba(255,255,255,0.25); border-radius: 4px; min-height: 40px;
        }
        QScrollBar::handle:vertical:hover { background: rgba(255,255,255,0.35); }
        QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0px; }
    )");

    auto* contentWidget = new QWidget(this);
    contentWidget->setStyleSheet("background: transparent;");
    auto* contentLayout = new QVBoxLayout(contentWidget);
    contentLayout->setSpacing(16);
    contentLayout->setContentsMargins(26, 22, 26, 26);

    auto* title = new QLabel("选择游戏模式", this);
    title->setStyleSheet(Style::labelStyle(Style::accentGold(), 22, true));
    title->setAlignment(Qt::AlignCenter);
    contentLayout->addWidget(title);

    auto* desc = new QLabel("挑选一个模式开始战斗", this);
    desc->setStyleSheet(Style::labelStyle(Style::textMuted(), 13, false));
    desc->setAlignment(Qt::AlignCenter);
    contentLayout->addWidget(desc);

    contentLayout->addSpacing(4);

    auto* grid = new QGridLayout();
    grid->setSpacing(14);
    grid->setContentsMargins(0, 0, 0, 0);

    struct CardDef {
        GameMode mode;
        QString label;   // short symbol
        QString accent;
        int row, col;
    };

    CardDef cards[] = {
        {GameMode::FreeMode,     "自\n由", "#4FC3F7", 0, 0},
        {GameMode::SpeedFree,    "极\n速", "#FFD700", 0, 1},
        {GameMode::TeamMode,     "团\n战", "#FF6B6B", 1, 0},
        {GameMode::BattleRoyale, "逃\n杀", "#CE93D8", 1, 1},
    };

    for (auto& cd : cards) {
        auto* card = makeModeCard(getModeConfig(cd.mode), cd.mode, cd.label, cd.accent);
        grid->addWidget(card, cd.row, cd.col);
    }

    contentLayout->addLayout(grid);

    auto* netCard = makeNetworkCard();
    contentLayout->addWidget(netCard);
    contentLayout->addStretch();

    scroll->setWidget(contentWidget);
    l->addWidget(scroll);
}

QFrame* ModeSelectWindow::makeModeCard(const GameModeConfig& cfg, GameMode mode,
                                       const QString& label, const QString& accent) {
    auto* card = new QFrame(this);
    card->setFixedHeight(128);
    card->setCursor(Qt::PointingHandCursor);
    card->setStyleSheet(QString(R"(
        QFrame#modeCard {
            background: rgba(18,28,52,0.80);
            border: 1px solid rgba(255,255,255,0.06);
            border-radius: 16px;
        }
        QFrame#modeCard:hover {
            background: rgba(18,28,52,0.94);
            border-color: %1;
        }
    )").arg(accent));
    card->setObjectName("modeCard");

    auto* shadow = new QGraphicsDropShadowEffect(card);
    shadow->setBlurRadius(20);
    shadow->setColor(QColor(0, 0, 0, 100));
    shadow->setOffset(0, 6);
    card->setGraphicsEffect(shadow);

    auto* mainLayout = new QHBoxLayout(card);
    mainLayout->setContentsMargins(16, 14, 16, 14);
    mainLayout->setSpacing(14);

    // 左侧图标区域 - 纯 CSS 无 emoji
    auto* iconFrame = new QFrame(card);
    iconFrame->setFixedSize(56, 56);
    iconFrame->setStyleSheet(QString(R"(
        QFrame {
            background: qradialgradient(cx:0.4,cy:0.35,radius:0.7,
                stop:0 %1, stop:1 %2);
            border-radius: 16px;
        }
    )").arg(accent).arg(accent + "55"));
    auto* iconLayout = new QVBoxLayout(iconFrame);
    iconLayout->setContentsMargins(0, 0, 0, 0);
    iconLayout->setSpacing(0);
    iconFrame->setLayout(iconLayout);
    auto* iconLabel = new QLabel(label, iconFrame);
    iconLabel->setAlignment(Qt::AlignCenter);
    iconLabel->setStyleSheet("font-size: 14px; font-weight: bold; color: white; background: transparent;");
    iconLayout->addWidget(iconLabel);
    mainLayout->addWidget(iconFrame);

    // 右侧文本
    auto* textLayout = new QVBoxLayout();
    textLayout->setSpacing(4);
    textLayout->setContentsMargins(0, 0, 0, 0);

    auto* nameLabel = new QLabel(cfg.name, card);
    nameLabel->setStyleSheet(QString("font-size: 17px; font-weight: bold; color: %1; background: transparent;").arg(accent));
    textLayout->addWidget(nameLabel);

    auto* descLabel = new QLabel(cfg.description, card);
    descLabel->setStyleSheet("font-size: 12px; color: #9090a0; background: transparent;");
    descLabel->setWordWrap(true);
    textLayout->addWidget(descLabel);

    auto* statsLabel = new QLabel(
        QString("地图 %1×%2  ·  AI ×%3  ·  %4分钟")
            .arg((int)cfg.worldWidth).arg((int)cfg.worldHeight)
            .arg(cfg.aiCount).arg(cfg.timeLimitSeconds / 60),
        card);
    statsLabel->setStyleSheet("font-size: 11px; color: #5a5a7a; background: transparent;");
    textLayout->addWidget(statsLabel);

    mainLayout->addLayout(textLayout, 1);

    card->installEventFilter(this);
    card->setProperty("mode", static_cast<int>(mode));

    return card;
}

QFrame* ModeSelectWindow::makeNetworkCard() {
    auto* card = new QFrame(this);
    card->setFixedHeight(88);
    card->setCursor(Qt::PointingHandCursor);
    card->setObjectName("netCard");
    card->setStyleSheet(R"(
        QFrame#netCard {
            background: qlineargradient(x1:0,y1:0,x2:1,y2:0,
                stop:0 rgba(79,195,247,0.10), stop:1 rgba(155,89,182,0.06));
            border: 1px solid rgba(79,195,247,0.20);
            border-radius: 16px;
        }
        QFrame#netCard:hover {
            background: qlineargradient(x1:0,y1:0,x2:1,y2:0,
                stop:0 rgba(79,195,247,0.18), stop:1 rgba(155,89,182,0.12));
            border-color: rgba(79,195,247,0.40);
        }
    )");

    auto* shadow = new QGraphicsDropShadowEffect(card);
    shadow->setBlurRadius(20);
    shadow->setColor(QColor(0, 0, 0, 100));
    shadow->setOffset(0, 6);
    card->setGraphicsEffect(shadow);

    auto* mainLayout = new QHBoxLayout(card);
    mainLayout->setContentsMargins(16, 14, 16, 14);
    mainLayout->setSpacing(14);

    // 图标
    auto* iconFrame = new QFrame(card);
    iconFrame->setFixedSize(52, 52);
    iconFrame->setStyleSheet(R"(
        QFrame {
            background: qradialgradient(cx:0.4,cy:0.35,radius:0.7,
                stop:0 rgba(79,195,247,0.6), stop:1 rgba(79,195,247,0.1));
            border-radius: 14px;
        }
    )");
    auto* iconLayout = new QVBoxLayout(iconFrame);
    iconLayout->setContentsMargins(0, 0, 0, 0);
    iconLayout->setSpacing(0);
    iconFrame->setLayout(iconLayout);
    auto* iconLabel = new QLabel("联\n网", iconFrame);
    iconLabel->setAlignment(Qt::AlignCenter);
    iconLabel->setStyleSheet("font-size: 14px; font-weight: bold; color: white; background: transparent;");
    iconLayout->addWidget(iconLabel);
    mainLayout->addWidget(iconFrame);

    auto* textLayout = new QVBoxLayout();
    textLayout->setSpacing(4);
    textLayout->setContentsMargins(0, 0, 0, 0);

    auto* nameLabel = new QLabel("联网对战", card);
    nameLabel->setStyleSheet("font-size: 17px; font-weight: bold; color: #4FC3F7; background: transparent;");
    textLayout->addWidget(nameLabel);

    auto* descLabel = new QLabel("连接 Go 服务端，与好友或其他玩家实时对战", card);
    descLabel->setStyleSheet("font-size: 12px; color: #9090a0; background: transparent;");
    descLabel->setWordWrap(true);
    textLayout->addWidget(descLabel);

    mainLayout->addLayout(textLayout, 1);

    card->installEventFilter(this);
    card->setProperty("network", true);

    return card;
}

bool ModeSelectWindow::eventFilter(QObject* watched, QEvent* event) {
    if (event->type() == QEvent::MouseButtonRelease) {
        auto* frame = qobject_cast<QFrame*>(watched);
        if (frame) {
            if (frame->property("network").toBool()) {
                promptNetworkAndEmit();
                return true;
            }
            int modeInt = frame->property("mode").toInt();
            emit modeSelected(static_cast<GameMode>(modeInt));
            return true;
        }
    }
    return SubWindow::eventFilter(watched, event);
}

void ModeSelectWindow::promptNetworkAndEmit() {
    auto& cfg = Config::instance();

    bool ok = false;
    QString host = QInputDialog::getText(this, QStringLiteral("服务器地址"),
        QStringLiteral("Host:"), QLineEdit::Normal,
        cfg.serverHost.isEmpty() ? "127.0.0.1" : cfg.serverHost, &ok);
    if (!ok || host.trimmed().isEmpty()) return;

    QString portStr = QInputDialog::getText(this, QStringLiteral("端口"),
        QStringLiteral("Port:"), QLineEdit::Normal,
        QString::number(cfg.serverPort > 0 ? cfg.serverPort : 8765), &ok);
    if (!ok || portStr.trimmed().isEmpty()) return;
    bool portOk = false;
    int port = portStr.trimmed().toInt(&portOk);
    if (!portOk || port <= 0 || port > 65535) return;

    QString name = QInputDialog::getText(this, QStringLiteral("玩家名"),
        QStringLiteral("Name:"), QLineEdit::Normal,
        cfg.playerName.isEmpty() ? "Player" : cfg.playerName, &ok);
    if (!ok || name.trimmed().isEmpty()) return;

    QString roomName = QInputDialog::getText(this, QStringLiteral("房间名"),
        QStringLiteral("Room (留空进默认房间，新名自动创建):"), QLineEdit::Normal,
        "default", &ok);
    if (!ok) return;
    roomName = roomName.trimmed();
    if (roomName.isEmpty()) roomName = "default";

    emit networkModeSelected(host.trimmed(), port, name.trimmed(), roomName, true, 20);
}
