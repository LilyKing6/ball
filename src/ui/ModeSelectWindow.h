#ifndef MODESELECTWINDOW_H
#define MODESELECTWINDOW_H

#include "SubWindow.h"
#include "engine/GameMode.h"
#include <QFrame>

class ModeSelectWindow : public SubWindow {
    Q_OBJECT
public:
    explicit ModeSelectWindow(QWidget* parent = nullptr);

signals:
    void modeSelected(GameMode mode);
    void networkModeSelected(const QString& host, int port, const QString& name,
                             const QString& roomName, bool createIfMissing, int capacity);

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;

private:
    QFrame* makeModeCard(const GameModeConfig& cfg, GameMode mode,
                         const QString& icon, const QString& accent);
    QFrame* makeNetworkCard();
    void promptNetworkAndEmit();
};

#endif // MODESELECTWINDOW_H
