#ifndef NETWORKCLIENT_H
#define NETWORKCLIENT_H

#include <QObject>
#include <QUrl>
#include <QString>
#include <QWebSocket>
#include <QTimer>
#include <QElapsedTimer>

#include "engine/WorldSnapshot.h"
#include "engine/IController.h"

struct RoomInfo {
    QString name;
    QString mode;
    int playerCount = 0;
    int capacity = 0;
};

// NetworkClient
//   连接 Go 服务端的 WebSocket 客户端封装。
//   协议（JSON over WebSocket）：
//     C → S：{"type":"join","payload":{"name":"X","mode":"free"}}
//            {"type":"input","payload":{"input":{"cursor":{...},"wantSplit":bool,"wantEject":bool}}}
//            {"type":"leave"}
//     S → C：{"type":"welcome","payload":{"playerId":1,"worldWidth":...,"worldHeight":...,"roomName":"..."}}
//            {"type":"snapshot","payload":{"tickId":N,"snapshot":{...}}}
//            {"type":"error","payload":{"message":"..."}}
class NetworkClient : public QObject {
    Q_OBJECT
public:
    explicit NetworkClient(QObject* parent = nullptr);
    ~NetworkClient() override;

    // 连接服务端并加入房间
    void connectTo(const QUrl& url, const QString& playerName, const QString& mode = "free");
    void closeConnection();

    // 发送 input（节流到约 30Hz，多次调用只取最新一帧）
    void sendInput(const PlayerInput& input);
    // 立即发送（用于 split/eject 这种 edge 事件）
    void sendInputImmediate(const PlayerInput& input);

    // 房间操作
    void listRooms();
    void createRoom(const QString& name, const QString& mode = "free", int capacity = 20);
    void joinRoom(const QString& roomName, const QString& playerName, const QString& mode = "free", bool createIfMissing = false, int capacity = 20);

    bool isConnected() const;
    int myPlayerId() const { return m_myPlayerId; }
    float worldWidth() const { return m_worldWidth; }
    float worldHeight() const { return m_worldHeight; }
    QString currentRoom() const { return m_currentRoom; }

signals:
    void connected();
    void welcomeReceived(int myPlayerId, float worldW, float worldH);
    // snapshot 接收时间用于客户端插值（msSinceEpoch）
    void snapshotReceived(const WorldSnapshot& snap, qint64 recvMsSinceEpoch);
    void roomListReceived(const QVector<RoomInfo>& rooms);
    void roomCreated(const QString& name, const QString& mode, int capacity);
    void roomJoined(const QString& name);
    void errorOccurred(const QString& message);
    void disconnected(const QString& reason);
    void deathReceived(const QString& message);

private slots:
    void onConnected();
    void onTextMessageReceived(const QString& msg);
    void onSocketDisconnected();
    void onSocketError(QAbstractSocket::SocketError);
    void onReconnectTimer();

private:
    void doConnect();
    void scheduleReconnect();
    void sendInputJson(const PlayerInput& input);

    QWebSocket m_socket;
    QUrl m_url;
    QString m_playerName;
    QString m_mode;

    int m_myPlayerId = 0;
    float m_worldWidth = 3000.0f;
    float m_worldHeight = 3000.0f;
    QString m_currentRoom;

    // 节流
    qint64 m_lastSentMs = 0;
    PlayerInput m_pendingThrottled;
    bool m_hasPendingThrottled = false;

    // 重连
    QTimer m_reconnectTimer;
    int m_reconnectAttempts = 0;
    bool m_intentionalClose = false;
};

#endif // NETWORKCLIENT_H
