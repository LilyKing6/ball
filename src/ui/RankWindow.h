#ifndef RANKWINDOW_H
#define RANKWINDOW_H

#include "SubWindow.h"
#include <QLabel>
#include <QProgressBar>

class RankWindow : public SubWindow {
    Q_OBJECT
public:
    explicit RankWindow(QWidget* parent = nullptr);

    void refresh(const QString& playerId);

private:
    QLabel* m_tierLabel;
    QLabel* m_eloLabel;
    QProgressBar* m_progressBar;
    QLabel* m_progressText;
    QLabel* m_seasonLabel;
};

#endif // RANKWINDOW_H
