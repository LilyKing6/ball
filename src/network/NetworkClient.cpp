#include "NetworkClient.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QDateTime>
#include <QDebug>

NetworkClient::NetworkClient(QObject* parent) : QObject(parent) {
    connect(&m_socket, &QWebSocket::connected, this, &NetworkClient::onConnected);
    connect(&m_socket, &QWebSocket::textMessageReceived, this, &NetworkClient::onTextMessageReceived);
    connect(&m_socket, &QWebSocket::disconnected, this, &NetworkClient::onSocketDisconnected);
    connect(&m_socket, QOverload<QAbstractSocket::SocketError>::of(&QWebSocket::errorOccurred),
            this, &NetworkClient::onSocketError);

    m_reconnectTimer.setSingleShot(true);
    connect(&m_reconnectTimer, &QTimer::timeout, this, &NetworkClient::onReconnectTimer);
}

NetworkClient::~NetworkClient() {
    closeConnection();
}

void NetworkClient::connectTo(const QUrl& url, const QString& playerName, const QString& mode) {
    // 兼容旧入口：直接连接后发送 join default
    m_url = url;
    m_playerName = playerName;
    m_mode = mode;
    m_currentRoom.clear();
    m_intentionalClose = false;
    m_reconnectAttempts = 0;
    doConnect();
}

void NetworkClient::listRooms() {
    if (!isConnected()) return;
    QJsonObject env;
    env["type"] = "list_rooms";
    env["payload"] = QJsonObject{};
    m_socket.sendTextMessage(QString::fromUtf8(QJsonDocument(env).toJson(QJsonDocument::Compact)));
}

void NetworkClient::createRoom(const QString& name, const QString& mode, int capacity) {
    if (!isConnected()) return;
    QJsonObject payload;
    payload["name"] = name;
    payload["mode"] = mode;
    payload["capacity"] = capacity;

    QJsonObject env;
    env["type"] = "create_room";
    env["payload"] = payload;
    m_socket.sendTextMessage(QString::fromUtf8(QJsonDocument(env).toJson(QJsonDocument::Compact)));
}

void NetworkClient::joinRoom(const QString& roomName, const QString& playerName, const QString& mode, bool createIfMissing, int capacity) {
    if (!isConnected()) return;
    m_playerName = playerName;
    m_mode = mode;
    m_currentRoom = roomName;

    QJsonObject payload;
    payload["name"] = playerName;
    payload["mode"] = mode;
    payload["room"] = roomName;
    payload["createIfMissing"] = createIfMissing;
    payload["capacity"] = capacity;

    QJsonObject env;
    env["type"] = "join";
    env["payload"] = payload;
    m_socket.sendTextMessage(QString::fromUtf8(QJsonDocument(env).toJson(QJsonDocument::Compact)));
}

void NetworkClient::doConnect() {
    qInfo() << "[Network] connecting to" << m_url.toString();
    m_socket.open(m_url);
}

void NetworkClient::closeConnection() {
    m_intentionalClose = true;
    m_reconnectTimer.stop();
    if (m_socket.state() != QAbstractSocket::UnconnectedState) {
        m_socket.close();
    }
}

bool NetworkClient::isConnected() const {
    return m_socket.state() == QAbstractSocket::ConnectedState;
}

void NetworkClient::onConnected() {
    qInfo() << "[Network] connected to" << m_url.toString();
    m_reconnectAttempts = 0;

    emit connected();
}

void NetworkClient::onTextMessageReceived(const QString& msg) {
    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(msg.toUtf8(), &err);
    if (err.error != QJsonParseError::NoError || !doc.isObject()) {
        qWarning() << "[Network] bad JSON:" << err.errorString();
        return;
    }
    QJsonObject env = doc.object();
    QString type = env.value("type").toString();
    QJsonObject payload = env.value("payload").toObject();

    if (type == "welcome") {
        m_myPlayerId = payload.value("playerId").toInt(0);
        m_worldWidth = static_cast<float>(payload.value("worldWidth").toDouble(3000));
        m_worldHeight = static_cast<float>(payload.value("worldHeight").toDouble(3000));
        m_currentRoom = payload.value("roomName").toString();
        qInfo() << "[Network] welcome: playerId=" << m_myPlayerId
                << ", world=" << m_worldWidth << "x" << m_worldHeight
                << ", room=" << m_currentRoom;
        emit welcomeReceived(m_myPlayerId, m_worldWidth, m_worldHeight);
    } else if (type == "snapshot") {
        // payload = {"tickId":N, "snapshot":{...}}
        QJsonObject snapObj = payload.value("snapshot").toObject();
        // 服务端可能将 snapshot 用 RawMessage 包成嵌套对象或直接 object
        if (snapObj.isEmpty()) {
            // 尝试作为顶级字段
            snapObj = payload;
        }
        WorldSnapshot snap = WorldSnapshot::fromJson(snapObj);
        qint64 nowMs = QDateTime::currentMSecsSinceEpoch();
        emit snapshotReceived(snap, nowMs);
    } else if (type == "room_list") {
        QJsonArray arr = payload.value("rooms").toArray();
        QVector<RoomInfo> rooms;
        for (const auto& v : arr) {
            QJsonObject o = v.toObject();
            RoomInfo info;
            info.name = o.value("name").toString();
            info.mode = o.value("mode").toString();
            info.playerCount = o.value("playerCount").toInt();
            info.capacity = o.value("capacity").toInt();
            rooms.append(info);
        }
        emit roomListReceived(rooms);
    } else if (type == "room_created") {
        QString name = payload.value("name").toString();
        QString mode = payload.value("mode").toString();
        int capacity = payload.value("capacity").toInt();
        emit roomCreated(name, mode, capacity);
    } else if (type == "room_joined") {
        m_currentRoom = payload.value("roomName").toString();
        emit roomJoined(m_currentRoom);
    } else if (type == "error") {
        QString errMsg = payload.value("message").toString();
        qWarning() << "[Network] server error:" << errMsg;
        emit errorOccurred(errMsg);
    } else if (type == "death") {
        QString deathMsg = payload.value("message").toString();
        qInfo() << "[Network] death:" << deathMsg;
        emit deathReceived(deathMsg);
    } else {
        qDebug() << "[Network] unknown message type:" << type;
    }
}

void NetworkClient::onSocketDisconnected() {
    qInfo() << "[Network] disconnected";
    if (m_intentionalClose) {
        emit disconnected("用户主动断开");
        return;
    }
    // 首次断线立即通知 UI 显示重连遮罩
    if (m_reconnectAttempts == 0) {
        emit disconnected("连接断开，正在重连...");
    }
    scheduleReconnect();
}

void NetworkClient::onSocketError(QAbstractSocket::SocketError) {
    qWarning() << "[Network] socket error:" << m_socket.errorString();
    if (m_intentionalClose) return;
    if (m_socket.state() == QAbstractSocket::UnconnectedState) {
        scheduleReconnect();
    }
}

void NetworkClient::scheduleReconnect() {
    m_reconnectAttempts++;
    if (m_reconnectAttempts > 3) {
        qWarning() << "[Network] reconnect attempts exhausted";
        emit disconnected("已断开连接（重连失败）");
        return;
    }
    int delayMs = 1000 * (1 << (m_reconnectAttempts - 1)); // 1s / 2s / 4s
    qInfo() << "[Network] reconnect attempt" << m_reconnectAttempts << "in" << delayMs << "ms";
    m_reconnectTimer.start(delayMs);
}

void NetworkClient::onReconnectTimer() {
    if (m_intentionalClose) return;
    doConnect();
}

void NetworkClient::sendInput(const PlayerInput& input) {
    qint64 now = QDateTime::currentMSecsSinceEpoch();
    if (now - m_lastSentMs >= 33) {
        sendInputJson(input);
        m_lastSentMs = now;
        m_hasPendingThrottled = false;
    } else {
        m_pendingThrottled = input;
        m_hasPendingThrottled = true;
    }
}

void NetworkClient::sendInputImmediate(const PlayerInput& input) {
    if (!isConnected()) return;
    sendInputJson(input);
    m_lastSentMs = QDateTime::currentMSecsSinceEpoch();
    m_hasPendingThrottled = false;
}

void NetworkClient::sendInputJson(const PlayerInput& input) {
    if (!isConnected()) return;

    QJsonObject inputObj;
    QJsonObject cursor;
    cursor["x"] = input.virtualCursor.x;
    cursor["y"] = input.virtualCursor.y;
    inputObj["cursor"] = cursor;
    inputObj["wantSplit"] = input.wantSplit;
    inputObj["wantEject"] = input.wantEject;

    QJsonObject mouse;
    mouse["x"] = input.mouseWorldPos.x;
    mouse["y"] = input.mouseWorldPos.y;
    inputObj["mouseWorld"] = mouse;

    QJsonObject payload;
    payload["input"] = inputObj;

    QJsonObject env;
    env["type"] = "input";
    env["payload"] = payload;

    m_socket.sendTextMessage(QString::fromUtf8(QJsonDocument(env).toJson(QJsonDocument::Compact)));
}
