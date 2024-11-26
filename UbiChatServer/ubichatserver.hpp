#pragma once

#include <QObject>
#include <QTcpSocket>
#include <QTcpServer>
#include <QHash>
#include <QTimer>
#include "ubichatsession.hpp"

class UbiChatServer : public QObject
{
    Q_OBJECT
public:
    explicit UbiChatServer(quint16 = 4546, QObject *parent = nullptr);
    ~UbiChatServer();

    bool start(quint16 listenPort);
    bool shutdown();

private:
    enum class CloseSessionReason {
        SHUTDOWN,
        KICK,
    };

    static constexpr uint ACTIVITY_TIMEOUT = 10000;

    void cleanUp();
    bool registerSession(UbiChatSession &origin);
    void closeSession(UbiChatSession &session, CloseSessionReason reason);

    quint16 _listen_port;
    QTcpServer _net_server;
    QHash<UbiChatSession*, UbiChatSession*> _sessions_pending;
    QHash<QString, UbiChatSession*> _sessions_registered;
    QTimer _cleanup_timer;

private slots:
    void handleNewConnection();
    void handleMessage(UbiChatSession &origin, const QString &message);
    void handleSessionDisconection(UbiChatSession &session);
};
