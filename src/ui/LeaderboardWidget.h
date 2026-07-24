#ifndef LEADERBOARDWIDGET_H
#define LEADERBOARDWIDGET_H

#include <QWidget>
#include <QLabel>
#include <QVBoxLayout>

class World;

class LeaderboardWidget : public QWidget {
    Q_OBJECT
public:
    explicit LeaderboardWidget(QWidget* parent = nullptr);

    void refresh(const World& world);

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    struct Entry {
        QString name;
        float mass;
        bool isLocal;
        int team = 0;
    };
    QVector<Entry> m_entries;
    bool m_teamMode = false;
    float m_teamAMass = 0;
    float m_teamBMass = 0;
    int m_localTeam = 0;
};

#endif // LEADERBOARDWIDGET_H
