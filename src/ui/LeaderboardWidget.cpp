#include "LeaderboardWidget.h"
#include "engine/World.h"
#include "entity/Player.h"
#include "util/Config.h"
#include <QPainter>
#include <algorithm>

LeaderboardWidget::LeaderboardWidget(QWidget* parent) : QWidget(parent) {
    setAttribute(Qt::WA_TransparentForMouseEvents);
    setFixedSize(180, 280);
    setStyleSheet("background: transparent;");
}

void LeaderboardWidget::refresh(const World& world) {
    m_entries.clear();
    const auto& players = world.players();
    const Player* local = world.localPlayer();

    m_teamMode = (world.currentMode() == GameMode::TeamMode);
    m_teamAMass = 0;
    m_teamBMass = 0;
    m_localTeam = local ? local->team : 0;

    for (const auto& p : players) {
        float m = p.totalMass();
        if (m <= 0) continue;
        m_entries.append({p.name, m, &p == local, p.team});
        if (p.team == 1) m_teamAMass += m;
        else if (p.team == 2) m_teamBMass += m;
    }

    if (m_teamMode) {
        // 团战：按本地队伍优先 + 队伍内质量降序
        std::sort(m_entries.begin(), m_entries.end(),
            [this](const Entry& a, const Entry& b) {
                bool aMyTeam = (a.team == m_localTeam);
                bool bMyTeam = (b.team == m_localTeam);
                if (aMyTeam != bMyTeam) return aMyTeam;  // 我方在前
                return a.mass > b.mass;
            });
    } else {
        std::sort(m_entries.begin(), m_entries.end(),
            [](const Entry& a, const Entry& b) { return a.mass > b.mass; });
    }

    if (m_entries.size() > Config::instance().leaderboardMaxEntries)
        m_entries.resize(Config::instance().leaderboardMaxEntries);
    update();
}

void LeaderboardWidget::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    // Background
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(0, 0, 0, 120));
    p.drawRoundedRect(rect(), 8, 8);

    // Title
    p.setPen(QColor("#FFD700"));
    QFont titleFont("Microsoft YaHei", 11, QFont::Bold);
    p.setFont(titleFont);

    int contentY = 30;

    if (m_teamMode) {
        // 团战标题：双方质量
        p.drawText(QRect(0, 5, width(), 22), Qt::AlignCenter, "团队战");

        QFont smallFont("Microsoft YaHei", 9, QFont::Bold);
        p.setFont(smallFont);

        // 蓝队（A）
        p.setPen(QColor(80, 150, 255));
        p.drawText(QRect(8, 28, 80, 16), Qt::AlignLeft, "蓝队");
        p.drawText(QRect(width() - 60, 28, 50, 16), Qt::AlignRight,
                   QString::number((int)m_teamAMass));

        // 蓝队进度条
        float total = qMax(1.0f, m_teamAMass + m_teamBMass);
        float aRatio = m_teamAMass / total;
        QRect aBar(8, 46, width() - 16, 5);
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(50, 50, 70));
        p.drawRoundedRect(aBar, 2, 2);
        p.setBrush(QColor(80, 150, 255));
        p.drawRoundedRect(QRect(aBar.x(), aBar.y(), int(aBar.width() * aRatio), aBar.height()), 2, 2);

        // 红队（B）
        p.setPen(QColor(255, 80, 80));
        p.setFont(smallFont);
        p.drawText(QRect(8, 56, 80, 16), Qt::AlignLeft, "红队");
        p.drawText(QRect(width() - 60, 56, 50, 16), Qt::AlignRight,
                   QString::number((int)m_teamBMass));

        QRect bBar(8, 74, width() - 16, 5);
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(50, 50, 70));
        p.drawRoundedRect(bBar, 2, 2);
        p.setBrush(QColor(255, 80, 80));
        float bRatio = m_teamBMass / total;
        p.drawRoundedRect(QRect(bBar.x(), bBar.y(), int(bBar.width() * bRatio), bBar.height()), 2, 2);

        // 分隔线
        p.setPen(QColor(255, 255, 255, 50));
        p.drawLine(8, 86, width() - 8, 86);

        contentY = 92;
    } else {
        p.drawText(QRect(0, 5, width(), 22), Qt::AlignCenter, "排行榜");
    }

    if (m_entries.isEmpty()) return;

    QFont entryFont("Microsoft YaHei", 9);
    p.setFont(entryFont);

    int y = contentY;
    int maxRows = (height() - contentY - 8) / 18;

    for (int i = 0; i < qMin(m_entries.size(), maxRows); i++) {
        const auto& e = m_entries[i];

        QColor textColor;
        if (e.isLocal) {
            textColor = QColor("#FFD700");
        } else if (m_teamMode && e.team == 1) {
            textColor = QColor(120, 180, 255);
        } else if (m_teamMode && e.team == 2) {
            textColor = QColor(255, 130, 130);
        } else {
            textColor = QColor(200, 200, 200);
        }
        p.setPen(textColor);

        QString rankStr = QString::number(i + 1);
        p.drawText(QRect(6, y, 20, 16), Qt::AlignLeft, rankStr);

        QString nameStr = e.name;
        if (nameStr.length() > 8) nameStr = nameStr.left(7) + "..";
        p.drawText(QRect(26, y, 100, 16), Qt::AlignLeft, nameStr);

        QString massStr = QString::number(static_cast<int>(e.mass));
        p.drawText(QRect(130, y, 44, 16), Qt::AlignRight, massStr);

        y += 18;
    }
}
