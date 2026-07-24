#include "GameOverScreen.h"
#include "Style.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QFrame>

static QFrame* makeStatCard(const QString& label, QLabel*& valueLabel, QWidget* parent,
                            const QString& accentColor = "#FFD700") {
    auto* card = new QFrame(parent);
    card->setStyleSheet(QString(R"(
        QFrame {
            background: rgba(255,255,255,0.05);
            border: 1px solid rgba(255,255,255,0.08);
            border-radius: 12px;
            padding: 12px;
        }
        QFrame:hover {
            border-color: %1;
            background: rgba(255,255,255,0.07);
        }
    )").arg(accentColor));

    auto* lay = new QVBoxLayout(card);
    lay->setSpacing(6);
    lay->setContentsMargins(0, 0, 0, 0);

    auto* lbl = new QLabel(label, card);
    lbl->setStyleSheet("font-size: 11px; color: #888; background: transparent;");
    lbl->setAlignment(Qt::AlignCenter);
    lay->addWidget(lbl);

    valueLabel = new QLabel("--", card);
    valueLabel->setStyleSheet(QString("font-size: 24px; font-weight: bold; color: %1; background: transparent;").arg(accentColor));
    valueLabel->setAlignment(Qt::AlignCenter);
    lay->addWidget(valueLabel);

    return card;
}

GameOverScreen::GameOverScreen(QWidget* parent)
    : SubWindow("游戏结束", parent, 500, 460) {

    auto* l = contentLayout();
    l->setSpacing(14);

    // 标题 + 勋章
    auto* titleCard = new QFrame(this);
    titleCard->setStyleSheet(QString(R"(
        QFrame {
            background: qradialgradient(cx:0.5,cy:0.5,radius:0.8,
                stop:0 rgba(255,255,255,0.08), stop:1 rgba(255,255,255,0.02));
            border: 1px solid rgba(255,255,255,0.1);
            border-radius: 16px;
        }
    )"));
    auto* titleLayout = new QVBoxLayout(titleCard);
    titleLayout->setSpacing(4);

    m_titleLabel = new QLabel("你被吃掉了！", this);
    m_titleLabel->setStyleSheet(Style::labelStyle(Style::accentRed(), 26, true));
    m_titleLabel->setAlignment(Qt::AlignCenter);
    titleLayout->addWidget(m_titleLabel);

    auto* medalLabel = new QLabel("🏅", this);
    medalLabel->setStyleSheet("font-size: 36px; background: transparent;");
    medalLabel->setAlignment(Qt::AlignCenter);
    titleLayout->addWidget(medalLabel);

    l->addWidget(titleCard);

    // 统计卡片区
    auto* grid = new QGridLayout();
    grid->setSpacing(10);
    grid->setContentsMargins(0, 0, 0, 0);

    grid->addWidget(makeStatCard("排名", m_rankValue, this, "#4FC3F7"), 0, 0);
    grid->addWidget(makeStatCard("最高质量", m_massValue, this, "#FFD700"), 0, 1);
    grid->addWidget(makeStatCard("存活时间", m_timeValue, this, "#888"), 0, 2);
    grid->addWidget(makeStatCard("击杀数", m_killsValue, this, "#FF6B6B"), 1, 0);
    grid->addWidget(makeStatCard("食物数", m_foodValue, this, "#2ecc71"), 1, 1);
    grid->addWidget(makeStatCard("ELO变化", m_eloValue, this, "#FFD700"), 1, 2);

    l->addLayout(grid);

    l->addSpacing(10);

    // 按钮行
    auto* btnRow = new QHBoxLayout();
    btnRow->setSpacing(16);
    btnRow->setAlignment(Qt::AlignCenter);

    auto* playAgainBtn = new QPushButton("再来一局", this);
    playAgainBtn->setFixedSize(170, 48);
    Style::applyPrimaryButton(playAgainBtn);
    btnRow->addWidget(playAgainBtn);

    auto* menuBtn = new QPushButton("返回主菜单", this);
    menuBtn->setFixedSize(170, 48);
    Style::applySecondaryButton(menuBtn);
    btnRow->addWidget(menuBtn);

    l->addLayout(btnRow);
    l->addStretch();

    connect(playAgainBtn, &QPushButton::clicked, this, &GameOverScreen::playAgain);
    connect(menuBtn, &QPushButton::clicked, this, &GameOverScreen::returnToMenu);
}

void GameOverScreen::showStats(const GameRecord& rec, int eloChange, bool victory) {
    if (victory) {
        m_titleLabel->setText("胜利！");
        m_titleLabel->setStyleSheet(Style::labelStyle(Style::accentGreen(), 26, true));
    } else {
        m_titleLabel->setText("你被吃掉了！");
        m_titleLabel->setStyleSheet(Style::labelStyle(Style::accentRed(), 26, true));
    }
    m_rankValue->setText(QString("#%1/%2").arg(rec.rankInMatch).arg(rec.totalPlayers));

    m_massValue->setText(QString::number(static_cast<int>(rec.maxMass)));

    int mins = static_cast<int>(rec.duration) / 60;
    int secs = static_cast<int>(rec.duration) % 60;
    m_timeValue->setText(QString("%1:%2").arg(mins).arg(secs, 2, 10, QChar('0')));

    m_killsValue->setText(QString::number(rec.killCount));

    m_foodValue->setText(QString::number(rec.foodEaten));

    QString eloText = eloChange >= 0 ? QString("+%1").arg(eloChange) : QString::number(eloChange);
    m_eloValue->setText(eloText);
    m_eloValue->setStyleSheet(QString("font-size: 24px; font-weight: bold; color: %1; background: transparent;")
        .arg(eloChange >= 0 ? "#2ecc71" : "#e74c3c"));
}
