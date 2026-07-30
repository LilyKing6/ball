#include "RankWindow.h"
#include "Style.h"
#include "ranking/RankSystem.h"
#include "ranking/SeasonManager.h"
#include "storage/DatabaseManager.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QSqlQuery>
#include <QFrame>
#include <QGraphicsDropShadowEffect>
#include <QScrollArea>

RankWindow::RankWindow(QWidget* parent)
    : SubWindow("段位赛季", parent, 460, 520) {

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

    // 段位大徽章
    auto* badge = new QFrame(this);
    badge->setStyleSheet(QString(R"(
        QFrame {
            background: qradialgradient(cx:0.5,cy:0.5,radius:0.8,
                stop:0 rgba(255,215,0,0.12), stop:1 rgba(255,215,0,0.02));
            border: 1px solid rgba(255,215,0,0.2);
            border-radius: 16px;
        }
    )"));
    auto* badgeLayout = new QVBoxLayout(badge);
    badgeLayout->setSpacing(6);

    m_tierLabel = new QLabel(this);
    m_tierLabel->setStyleSheet(Style::labelStyle(Style::accentGold(), 32, true));
    m_tierLabel->setAlignment(Qt::AlignCenter);
    badgeLayout->addWidget(m_tierLabel);

    m_eloLabel = new QLabel(this);
    m_eloLabel->setStyleSheet(Style::labelStyle(Style::textSecondary(), 16, false));
    m_eloLabel->setAlignment(Qt::AlignCenter);
    badgeLayout->addWidget(m_eloLabel);

    m_progressBar = new QProgressBar(this);
    m_progressBar->setRange(0, 1000);
    m_progressBar->setTextVisible(false);
    m_progressBar->setFixedHeight(14);
    m_progressBar->setStyleSheet(Style::sliderStyle(Style::accentGold()) +
        "QProgressBar { background: rgba(255,255,255,0.1); border-radius: 7px; } QProgressBar::chunk { border-radius: 7px; }");
    badgeLayout->addWidget(m_progressBar);

    m_progressText = new QLabel(this);
    m_progressText->setStyleSheet(Style::labelStyle(Style::textMuted(), 12, false));
    m_progressText->setAlignment(Qt::AlignCenter);
    badgeLayout->addWidget(m_progressText);

    m_seasonLabel = new QLabel(this);
    m_seasonLabel->setStyleSheet(Style::labelStyle(Style::textMuted(), 12, false));
    m_seasonLabel->setAlignment(Qt::AlignCenter);
    badgeLayout->addWidget(m_seasonLabel);

    contentLayout->addWidget(badge);

    // 段位阶梯
    auto* ladderTitle = new QLabel("🏆 段位阶梯", this);
    ladderTitle->setStyleSheet(Style::labelStyle(Style::accentGold(), 16, true));
    contentLayout->addWidget(ladderTitle);

    auto* ladderGrid = new QGridLayout();
    ladderGrid->setSpacing(6);
    ladderGrid->setContentsMargins(0, 0, 0, 0);

    struct LadderItem { QString name; int min; int max; QColor color; };
    QVector<LadderItem> ladder = {
        {"超神", 3500, 9999, QColor("#FF00FF")},
        {"王者", 3000, 3499, QColor("#FF0000")},
        {"大师", 2500, 2999, QColor("#FF4500")},
        {"钻石", 2000, 2499, QColor("#B9F2FF")},
        {"铂金", 1600, 1999, QColor("#E5E4E2")},
        {"黄金", 1200, 1599, QColor("#FFD700")},
        {"白银", 800, 1199, QColor("#C0C0C0")},
        {"青铜", 0, 799, QColor("#CD7F32")},
    };

    int row = 0;
    for (const auto& item : ladder) {
        auto* bar = new QFrame(this);
        bar->setFixedHeight(28);
        bar->setStyleSheet(QString(R"(
            QFrame {
                background: rgba(255,255,255,0.05);
                border-left: 4px solid %1;
                border-radius: 4px;
            }
        )").arg(item.color.name()));
        auto* barLayout = new QHBoxLayout(bar);
        barLayout->setContentsMargins(10, 0, 10, 0);

        auto* nameLabel = new QLabel(item.name, this);
        nameLabel->setStyleSheet(QString("font-size: 13px; font-weight: bold; color: %1; background: transparent;").arg(item.color.name()));
        barLayout->addWidget(nameLabel);

        barLayout->addStretch();

        auto* rangeLabel = new QLabel(QString("%1-%2").arg(item.min).arg(item.max == 9999 ? "∞" : QString::number(item.max)), this);
        rangeLabel->setStyleSheet("font-size: 11px; color: #666; background: transparent;");
        barLayout->addWidget(rangeLabel);

        ladderGrid->addWidget(bar, row++, 0);
    }

    contentLayout->addLayout(ladderGrid);
    contentLayout->addStretch();

    scroll->setWidget(contentWidget);
    l->addWidget(scroll);
}

void RankWindow::refresh(const QString& playerId) {
    auto& db = DatabaseManager::instance().database();
    QSqlQuery q(db);
    q.prepare("SELECT elo, rank_tier FROM player_profile WHERE player_id = ?");
    q.addBindValue(playerId);

    int elo = 1000;
    QString tierName;
    if (q.exec() && q.next()) {
        elo = q.value("elo").toInt();
        tierName = q.value("rank_tier").toString();
    }

    TierInfo info = RankSystem::tierInfoForElo(elo);
    float progress = RankSystem::progressToNextTier(elo);

    m_tierLabel->setText(info.name);
    m_tierLabel->setStyleSheet(Style::labelStyle(info.color, 32, true));

    m_eloLabel->setText(QString("ELO: %1").arg(elo));

    m_progressBar->setValue(static_cast<int>(progress * 1000));

    RankTier currentTier = RankSystem::tierFromElo(elo);
    if (currentTier == RankTier::SuperGod) {
        m_progressText->setText("已达最高段位");
    } else {
        TierInfo nextInfo = RankSystem::tierInfo(static_cast<RankTier>(static_cast<int>(currentTier) + 1));
        int needed = nextInfo.minElo - elo;
        m_progressText->setText(QString("距 %1 还需 %2 ELO").arg(nextInfo.name).arg(needed));
    }

    QString gradient = QString("stop:0 %1,stop:1 %2").arg(info.color.name()).arg(
        currentTier == RankTier::SuperGod ? info.color.name() :
        RankSystem::tierInfo(static_cast<RankTier>(static_cast<int>(currentTier) + 1)).color.name());
    m_progressBar->setStyleSheet(QString(R"(
        QProgressBar { background: rgba(255,255,255,0.1); border-radius: 7px; border: none; }
        QProgressBar::chunk { background: qlineargradient(x1:0,y1:0,x2:1,y2:0,%1); border-radius: 7px; }
    )").arg(gradient));

    auto& sm = SeasonManager::instance();
    m_seasonLabel->setText(QString("%1 赛季 · 剩余 %2 天").arg(sm.currentSeasonId()).arg(sm.daysRemaining()));
}
