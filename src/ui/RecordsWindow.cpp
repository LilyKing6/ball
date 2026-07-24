#include "RecordsWindow.h"
#include "Style.h"
#include "record/RecordManager.h"
#include <QLabel>
#include <QHBoxLayout>
#include <QFrame>

RecordsWindow::RecordsWindow(QWidget* parent)
    : SubWindow("游戏记录", parent, 520, 480) {

    auto* l = contentLayout();
    l->setSpacing(14);

    // 标题
    auto* title = new QLabel("📋 最近对局", this);
    title->setStyleSheet(Style::labelStyle(Style::accentGold(), 18, true));
    l->addWidget(title);

    m_list = new QListWidget(this);
    m_list->setStyleSheet(Style::listWidgetStyle());
    l->addWidget(m_list);
}

void RecordsWindow::loadRecords() {
    m_list->clear();
    auto records = RecordManager::instance().loadRecords("local", 10);
    for (auto& r : records) {
        QString modeStr;
        if (r.mode == "free") modeStr = "自由";
        else if (r.mode == "speed") modeStr = "极速";
        else if (r.mode == "team") modeStr = "团战";
        else if (r.mode == "battleroyale") modeStr = "逃杀";
        else modeStr = "单机";

        QString text = QString("[%1] %2 · 时长 %3s · 最大质量 %4 · 击杀 %5")
            .arg(r.timestamp.toString("MM-dd hh:mm"))
            .arg(modeStr)
            .arg((int)r.duration)
            .arg((int)r.maxMass)
            .arg(r.killCount);
        m_list->addItem(text);
    }
    if (records.isEmpty()) {
        m_list->addItem("暂无游戏记录");
    }
}
