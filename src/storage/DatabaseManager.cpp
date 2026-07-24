#include "DatabaseManager.h"
#include "Schema.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>

DatabaseManager& DatabaseManager::instance() {
    static DatabaseManager s;
    return s;
}

bool DatabaseManager::initialize(const QString& dbPath) {
    m_db = QSqlDatabase::addDatabase("QSQLITE");
    m_db.setDatabaseName(dbPath);

    if (!m_db.open()) {
        qCritical() << "Failed to open database:" << m_db.lastError().text();
        return false;
    }

    QSqlQuery q(m_db);
    QStringList statements = Schema::createTables().split(';');
    for (const auto& stmt : statements) {
        QString trimmed = stmt.trimmed();
        if (trimmed.isEmpty()) continue;
        if (!q.exec(trimmed)) {
            qWarning() << "Schema error:" << q.lastError().text();
        }
    }

    qDebug() << "Database initialized:" << dbPath;
    return true;
}
