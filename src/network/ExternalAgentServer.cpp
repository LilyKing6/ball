#include "ExternalAgentServer.h"
#include <QWebSocketServer>
#include <QWebSocket>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDebug>

ExternalAgentServer::ExternalAgentServer(quint16 port, QObject* parent)
    : QObject(parent) {
    m_server = new QWebSocketServer("BallBattleAgentServer",
                                     QWebSocketServer::NonSecureMode, this);
    if (m_server->listen(QHostAddress::LocalHost, port)) {
        qDebug() << "[AgentServer] listening on ws://127.0.0.1:" << port;
        connect(m_server, &QWebSocketServer::newConnection,
                this, &ExternalAgentServer::onNewConnection);
    } else {
        qWarning() << "[AgentServer] failed to listen on port" << port
                   << ":" << m_server->errorString();
    }
}

ExternalAgentServer::~ExternalAgentServer() {
    for (auto* c : m_clients) {
        c->close();
        c->deleteLater();
    }
    m_clients.clear();
    if (m_server) {
        m_server->close();
        m_server->deleteLater();
    }
}

bool ExternalAgentServer::isListening() const {
    return m_server && m_server->isListening();
}

int ExternalAgentServer::clientCount() const {
    return m_clients.size();
}

void ExternalAgentServer::onNewConnection() {
    while (m_server->hasPendingConnections()) {
        QWebSocket* client = m_server->nextPendingConnection();
        qDebug() << "[AgentServer] new client connected:" << client->peerAddress().toString();
        connect(client, &QWebSocket::textMessageReceived,
                this, &ExternalAgentServer::onTextMessageReceived);
        connect(client, &QWebSocket::disconnected,
                this, &ExternalAgentServer::onClientDisconnected);
        m_clients.append(client);
    }
}

void ExternalAgentServer::onTextMessageReceived(const QString& msg) {
    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(msg.toUtf8(), &err);
    if (err.error != QJsonParseError::NoError || !doc.isObject()) {
        qWarning() << "[AgentServer] invalid JSON from client:" << err.errorString();
        return;
    }
    QJsonObject obj = doc.object();
    int playerId = obj.value("playerId").toInt(0);
    QJsonObject inputObj = obj.value("input").toObject();
    if (!inputObj.isEmpty()) {
        emit inputReceived(playerId, inputObj);
    } else {
        // 兼容：直接把整个 obj 当作 input
        emit inputReceived(playerId, obj);
    }
}

void ExternalAgentServer::onClientDisconnected() {
    QWebSocket* client = qobject_cast<QWebSocket*>(sender());
    if (client) {
        m_clients.removeAll(client);
        client->deleteLater();
        qDebug() << "[AgentServer] client disconnected, remaining:" << m_clients.size();
    }
}

void ExternalAgentServer::broadcastSnapshot(const QJsonObject& snapshotJson) {
    if (m_clients.isEmpty()) return;
    QJsonDocument doc(snapshotJson);
    QString msg = QString::fromUtf8(doc.toJson(QJsonDocument::Compact));
    for (auto* client : m_clients) {
        if (client->state() == QAbstractSocket::ConnectedState) {
            client->sendTextMessage(msg);
        }
    }
}
