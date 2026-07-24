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

ModeSelectWindow::ModeSelectWindow(QWidget* parent)
    : SubWindow("选择模式", parent, 520, 560) {

    auto* l = contentLayout();
    l->setSpacing(0);
    l->setContentsMargins(0, 0, 0, 0);

    // 滚动区域
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

    auto* title = new QLabel("选择游戏模式", this);
    title->setStyleSheet(Style::labelStyle(Style::accentGold(), 20, true));
    title->setAlignment(Qt::AlignCenter);
    contentLayout->addWidget(title);

    // 2x2 模式卡片网格
    auto* grid = new QGridLayout();
    grid->setSpacing(12);
    grid->setContentsMargins(0, 0, 0, 0);

    auto addCard = [&](GameMode mode, const QString& icon, const QString& accent, int row, int col) {
        auto* card = makeModeCard(getModeConfig(mode), mode, icon, accent);
        grid->addWidget(card, row, col);
    };

    addCard(GameMode::FreeMode,    "◉", "#4FC3F7", 0, 0);
    addCard(GameMode::SpeedFree,   "⚡", "#FFD700", 0, 1);
    addCard(GameMode::TeamMode,    "⚔", "#FF6B6B", 1, 0);
    addCard(GameMode::BattleRoyale,"⚠", "#9C27B0", 1, 1);

    contentLayout->addLayout(grid);

    // 联网对战卡片（全宽）
    auto* netCard = makeNetworkCard();
    contentLayout->addWidget(netCard);

    contentLayout->addStretch();

    scroll->setWidget(contentWidget);
    l->addWidget(scroll);
}

QFrame* ModeSelectWindow::makeModeCard(const GameModeConfig& cfg, GameMode mode,
                                       const QString& icon, const QString& accent) {
    auto* card = new QFrame(this);
    card->setFixedHeight(118);
    card->setCursor(Qt::PointingHandCursor);
    card->setStyleSheet(QString(R"(
        QFrame {
            background: rgba(22,33,62,0.75);
            border: 1px solid rgba(255,255,255,0.08);
            border-radius: 14px;
        }
        QFrame:hover {
            background: rgba(22,33,62,0.92);
            border-color: %1;
        }
    )").arg(accent));

    auto* shadow = new QGraphicsDropShadowEffect(card);
    shadow->setBlurRadius(16);
    shadow->setColor(QColor(0, 0, 0, 80));
    shadow->setOffset(0, 4);
    card->setGraphicsEffect(shadow);

    auto* mainLayout = new QHBoxLayout(card);
    mainLayout->setContentsMargins(14, 10, 14, 10);
    mainLayout->setSpacing(12);

    // 左侧图标
    auto* iconLabel = new QLabel(icon, card);
    iconLabel->setFixedSize(48, 48);
    iconLabel->setAlignment(Qt::AlignCenter);
    iconLabel->setStyleSheet(QString(R"(
        QLabel {
            background: %1;
            border-radius: 24px;
            font-size: 22px;
            color: white;
        }
    )").arg(accent));
    mainLayout->addWidget(iconLabel);

    // 右侧文本
    auto* textLayout = new QVBoxLayout();
    textLayout->setSpacing(3);
    textLayout->setContentsMargins(0, 0, 0, 0);

    auto* nameLabel = new QLabel(cfg.name, card);
    nameLabel->setStyleSheet(QString("font-size: 16px; font-weight: bold; color: %1; background: transparent;").arg(accent));
    textLayout->addWidget(nameLabel);

    auto* descLabel = new QLabel(cfg.description, card);
    descLabel->setStyleSheet("font-size: 11px; color: #888; background: transparent;");
    descLabel->setWordWrap(true);
    textLayout->addWidget(descLabel);

    auto* statsLabel = new QLabel(QString("地图 %1×%2 · AI %3 · 限时 %4分钟")
        .arg((int)cfg.worldWidth).arg((int)cfg.worldHeight)
        .arg(cfg.aiCount).arg(cfg.timeLimitSeconds / 60), card);
    statsLabel->setStyleSheet("font-size: 10px; color: #666; background: transparent;");
    textLayout->addWidget(statsLabel);

    mainLayout->addLayout(textLayout, 1);

    card->installEventFilter(this);
    card->setProperty("mode", static_cast<int>(mode));

    return card;
}

QFrame* ModeSelectWindow::makeNetworkCard() {
    auto* card = new QFrame(this);
    card->setFixedHeight(82);
    card->setCursor(Qt::PointingHandCursor);
    card->setStyleSheet(R"(
        QFrame {
            background: qlineargradient(x1:0,y1:0,x2:1,y2:0,stop:0 rgba(79,195,247,0.12),stop:1 rgba(155,89,182,0.08));
            border: 1px solid rgba(79,195,247,0.25);
            border-radius: 14px;
        }
        QFrame:hover {
            background: qlineargradient(x1:0,y1:0,x2:1,y2:0,stop:0 rgba(79,195,247,0.20),stop:1 rgba(155,89,182,0.14));
            border-color: rgba(79,195,247,0.45);
        }
    )");

    auto* shadow = new QGraphicsDropShadowEffect(card);
    shadow->setBlurRadius(16);
    shadow->setColor(QColor(0, 0, 0, 80));
    shadow->setOffset(0, 4);
    card->setGraphicsEffect(shadow);

    auto* mainLayout = new QHBoxLayout(card);
    mainLayout->setContentsMargins(14, 10, 14, 10);
    mainLayout->setSpacing(12);

    auto* iconLabel = new QLabel("🌐", card);
    iconLabel->setFixedSize(44, 44);
    iconLabel->setAlignment(Qt::AlignCenter);
    iconLabel->setStyleSheet(R"(
        QLabel {
            background: qradialgradient(cx:0.5,cy:0.5,radius:0.8,
                stop:0 rgba(79,195,247,0.5), stop:1 rgba(79,195,247,0.1));
            border-radius: 22px;
            font-size: 20px;
            color: white;
        }
    )");
    mainLayout->addWidget(iconLabel);

    auto* textLayout = new QVBoxLayout();
    textLayout->setSpacing(3);
    textLayout->setContentsMargins(0, 0, 0, 0);

    auto* nameLabel = new QLabel("联网对战", card);
    nameLabel->setStyleSheet("font-size: 16px; font-weight: bold; color: #4FC3F7; background: transparent;");
    textLayout->addWidget(nameLabel);

    auto* descLabel = new QLabel("连接 Go 服务端，与好友或其他玩家实时对战", card);
    descLabel->setStyleSheet("font-size: 11px; color: #888; background: transparent;");
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
