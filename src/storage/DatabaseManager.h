#ifndef DATABASEMANAGER_H
#define DATABASEMANAGER_H

#include <QSqlDatabase>
#include <QString>

class DatabaseManager {
public:
    static DatabaseManager& instance();

    bool initialize(const QString& dbPath = "ballbattle.db");
    QSqlDatabase& database() { return m_db; }
    bool isOpen() const { return m_db.isOpen(); }

private:
    DatabaseManager() = default;
    QSqlDatabase m_db;
};

#endif // DATABASEMANAGER_H
