#include "AchievementWindow.h"
#include "Style.h"
#include "achievement/AchievementManager.h"
#include <QLabel>
#include <QFrame>
#include <QHBoxLayout>
#include <QProgressBar>

AchievementWindow::AchievementWindow(QWidget* parent)
    : SubWindow("成就", parent, 500, 520) {

    auto* l = contentLayout();
    l->setSpacing(10);

    auto* scroll = new QScrollArea(this);
    scroll->setWidgetResizable(true);
    scroll->setStyleSheet("QScrollArea { background: transparent; border: none; } QScrollBar:vertical { background: #1a1a2e; width: 8px; } QScrollBar::handle:vertical { background: #444; border-radius: 4px; }");

    m_contentWidget = new QWidget();
    m_contentWidget->setStyleSheet("background: transparent;");
    m_contentLayout = new QVBoxLayout(m_contentWidget);
    m_contentLayout->setSpacing(10);
    m_contentLayout->setContentsMargins(0, 0, 0, 0);
    m_contentLayout->addStretch();

    scroll->setWidget(m_contentWidget);
    l->addWidget(scroll);
}

void AchievementWindow::refresh() {
    // Clear existing items (keep the stretch at the end)
    while (m_contentLayout->count() > 1) {
        auto* item = m_contentLayout->takeAt(0);
        if (item->widget()) delete item->widget();
        delete item;
    }

    auto& ach = AchievementManager::instance();
    int unlocked = ach.unlockedCount();
    int total = ach.definitions().size();

    // 顶部进度卡片
    auto* header = new QFrame(this);
    header->setStyleSheet(Style::cardStyle());
    auto* headerLayout = new QVBoxLayout(header);
    headerLayout->setSpacing(6);

    auto* headerTitle = new QLabel(QString("⭐ 成就进度 %1 / %2").arg(unlocked).arg(total), this);
    headerTitle->setStyleSheet(Style::labelStyle(Style::accentGold(), 16, true));
    headerLayout->addWidget(headerTitle);

    auto* progressBar = new QProgressBar(this);
    progressBar->setRange(0, total);
    progressBar->setValue(unlocked);
    progressBar->setTextVisible(false);
    progressBar->setFixedHeight(10);
    progressBar->setStyleSheet(Style::sliderStyle(Style::accentGold()) +
        "QProgressBar { background: rgba(255,255,255,0.1); border-radius: 5px; } QProgressBar::chunk { border-radius: 5px; }");
    headerLayout->addWidget(progressBar);

    m_contentLayout->insertWidget(0, header);

    for (const auto& def : ach.definitions()) {
        auto* row = new QFrame(this);
        row->setStyleSheet(def.unlocked
            ? "QFrame { background: rgba(255,215,0,0.08); border: 1px solid rgba(255,215,0,0.2); border-radius: 10px; padding: 8px; }"
            : "QFrame { background: rgba(255,255,255,0.03); border: 1px solid rgba(255,255,255,0.06); border-radius: 10px; padding: 8px; }");
        auto* rowLayout = new QHBoxLayout(row);
        rowLayout->setSpacing(12);
        rowLayout->setContentsMargins(10, 8, 10, 8);

        // Icon placeholder
        auto* icon = new QLabel(this);
        icon->setFixedSize(40, 40);
        icon->setAlignment(Qt::AlignCenter);
        if (def.unlocked) {
            icon->setStyleSheet(QString("background: %1; border-radius: 20px; font-size: 16px; color: white;").arg("#FFD700"));
            icon->setText("★");
        } else {
            icon->setStyleSheet("background: #333; border-radius: 20px; font-size: 16px; color: #666;");
            icon->setText("?");
        }
        rowLayout->addWidget(icon);

        // Name + description
        auto* textLayout = new QVBoxLayout();
        textLayout->setSpacing(2);
        auto* nameLabel = new QLabel(def.unlocked ? def.name : "???", this);
        nameLabel->setStyleSheet(def.unlocked ? "font-size: 14px; font-weight: bold; color: #FFD700;" : "font-size: 14px; color: #555;");
        textLayout->addWidget(nameLabel);

        auto* descLabel = new QLabel(def.unlocked ? def.description : "达成条件后解锁", this);
        descLabel->setStyleSheet(def.unlocked ? "font-size: 11px; color: #999;" : "font-size: 11px; color: #444;");
        textLayout->addWidget(descLabel);
        rowLayout->addLayout(textLayout, 1);

        // Reward badge
        QString rewardText;
        switch (def.rewardType) {
        case RewardType::Title:  rewardText = "称号"; break;
        case RewardType::Skin:   rewardText = "皮肤"; break;
        case RewardType::Glow:   rewardText = "光效"; break;
        case RewardType::Outline: rewardText = "描边"; break;
        }
        auto* rewardLabel = new QLabel(rewardText, this);
        rewardLabel->setStyleSheet(def.unlocked
            ? "font-size: 11px; color: #FFD700; background: rgba(255,215,0,0.15); padding: 3px 8px; border-radius: 10px;"
            : "font-size: 11px; color: #444; background: rgba(255,255,255,0.05); padding: 3px 8px; border-radius: 10px;");
        rewardLabel->setAlignment(Qt::AlignCenter);
        rowLayout->addWidget(rewardLabel);

        m_contentLayout->insertWidget(m_contentLayout->count() - 1, row);
    }
}
