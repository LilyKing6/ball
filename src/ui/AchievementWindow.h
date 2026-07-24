#ifndef ACHIEVEMENTWINDOW_H
#define ACHIEVEMENTWINDOW_H

#include "SubWindow.h"
#include <QScrollArea>
#include <QVBoxLayout>

class AchievementWindow : public SubWindow {
    Q_OBJECT
public:
    explicit AchievementWindow(QWidget* parent = nullptr);
    void refresh();

private:
    QWidget* m_contentWidget;
    QVBoxLayout* m_contentLayout;
};

#endif // ACHIEVEMENTWINDOW_H
