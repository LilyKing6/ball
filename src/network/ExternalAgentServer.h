#ifndef EXTERNALAGENTSERVER_H
#define EXTERNALAGENTSERVER_H

#include <QObject>
#include <QJsonObject>
#include <QVector>

class QWebSocketServer;
class QWebSocket;

// WebSocket 服务端：外部 Agent（RL / 网络对战客户端）通过此通道接收 snapshot 并发送 PlayerInput
//
// 协议（JSON over WebSocket text frame）：
//   服务端 -> 客户端：WorldSnapshot 序列化（每 N tick 一帧）
//   客户端 -> 服务端：PlayerInput 序列化（{virtualCursor:{x,y}, wantSplit:bool, wantEject:bool, playerId:int}）
class ExternalAgentServer : public QObject {
    Q_OBJECT
public:
    explicit ExternalAgentServer(quint16 port = 8765, QObject* parent = nullptr);
    ~ExternalAgentServer() override;

    bool isListening() const;
    int clientCount() const;

    // 广播 snapshot 给所有连接的 agent
    void broadcastSnapshot(const QJsonObject& snapshotJson);

signals:
    // 收到某 agent 的 PlayerInput
    //   playerId: agent 自报玩家 id（若 0 = 默认本地玩家占位符）
    void inputReceived(int playerId, const QJsonObject& inputJson);

private slots:
    void onNewConnection();
    void onTextMessageReceived(const QString& msg);
    void onClientDisconnected();

private:
    QWebSocketServer* m_server = nullptr;
    QVector<QWebSocket*> m_clients;
};

#endif // EXTERNALAGENTSERVER_H
