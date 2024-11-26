#include "ubichatserver.hpp"

#include <QDebug>

UbiChatServer::UbiChatServer(quint16 port, QObject *parent)
    : QObject{parent}
    , _listen_port{port}
{
    connect(&_net_server, &QTcpServer::newConnection, this, &UbiChatServer::handleNewConnection);
    connect(&_cleanup_timer, &QTimer::timeout, this, &UbiChatServer::cleanUp);
}

UbiChatServer::~UbiChatServer()
{
    shutdown();
}

bool UbiChatServer::start(quint16 listenPort)
{
    if (_net_server.isListening())
        return true;

    _listen_port = listenPort;
    bool res = _net_server.listen(QHostAddress::Any, _listen_port);
    if (!res) {
        qDebug() << "Unable to start server: " << _net_server.errorString();
    } else {
        _cleanup_timer.start(1000);
    }
    qDebug() << "Start listening on " << _listen_port;
    return res;
}

bool UbiChatServer::shutdown()
{
    if (!_net_server.isListening())
        return true;

    qDebug() << "server shutting down";

    _cleanup_timer.stop();
    _net_server.close();

    for (auto i = _sessions_pending.begin(), end = _sessions_pending.end(); i != end; ++i) {
        closeSession(*i.value(), CloseSessionReason::SHUTDOWN);
    }

    for (auto i = _sessions_registered.begin(), end = _sessions_registered.end(); i != end; ++i) {
        closeSession(*i.value(), CloseSessionReason::SHUTDOWN);
    }

    return true;
}

void UbiChatServer::cleanUp()
{
    for (auto i = _sessions_pending.begin(), end = _sessions_pending.end(); i != end; ++i) {
        if (i.value()->activityTime().msecsTo(QTime::currentTime()) > ACTIVITY_TIMEOUT)
            closeSession(*i.value(), CloseSessionReason::KICK);
    }

    // qDebug() << "sessions_new count: " << _sessions_new.size();

    for (auto i = _sessions_registered.begin(), end = _sessions_registered.end(); i != end; ++i) {
        if (i.value()->activityTime().msecsTo(QTime::currentTime()) > ACTIVITY_TIMEOUT)
            closeSession(*i.value(), CloseSessionReason::KICK);
    }

    // qDebug() << "sessions_registered count: " << _sessions_registered.size();
}

bool UbiChatServer::registerSession(UbiChatSession &session)
{
    if (!session.nickname().length())
        return false;

    if (_sessions_registered.contains(session.nickname()))
        return false;

    _sessions_pending.remove(&session);
    session.registrate();
    _sessions_registered.insert(session.nickname(), &session);

    return true;
}

void UbiChatServer::closeSession(UbiChatSession &session, CloseSessionReason reason)
{
    QString reasonMessage;
    QString nick = session.isRegistered() ? session.nickname() : "unregistred";
    switch (reason) {
    case CloseSessionReason::SHUTDOWN:
        reasonMessage = "server shutdown";
        break;
    case CloseSessionReason::KICK :
        reasonMessage = QString("%1 was kicked due inactivity").arg(nick);
        break;
    default:
        reasonMessage = "server disconnect, unknown reason";
        break;
    }

    qDebug() << "close session by: " << reasonMessage;

    if (session.isRegistered()) {
        handleMessage(session, reasonMessage);
        session.sendMessage(reasonMessage);
        _sessions_registered.remove(session.nickname());
    } else {
        session.sendMessage(reasonMessage);
        _sessions_pending.remove(&session);
    }

    session.deleteLater();
}

void UbiChatServer::handleNewConnection()
{
    while (_net_server.hasPendingConnections()) {
        QTcpSocket *clientConnection = _net_server.nextPendingConnection();
        UbiChatSession *session = new UbiChatSession(clientConnection, this);
        _sessions_pending.insert(session, session);

        connect(session, &UbiChatSession::messageReceived, this, &UbiChatServer::handleMessage);
        connect(session,
                &UbiChatSession::sessionDisconected,
                this,
                &UbiChatServer::handleSessionDisconection);

        qDebug() << "new session created from " << clientConnection->peerAddress();
        qDebug() << QString("new session created from %2")
                        .arg(clientConnection->peerAddress().toString());
    }
}

void UbiChatServer::handleMessage(UbiChatSession &origin, const QString &message)
{
    // try to register
    if (!origin.isRegistered() && !registerSession(origin)) {
        auto reply = QString("Unable to register session with %1 nickname").arg(origin.nickname());
        origin.sendMessage(reply);
        qDebug() << reply;
        return;
    }

    // broadcast command if registered
    qDebug() << "broadcast message: " << message;
    for (auto session : std::as_const(_sessions_registered)) {
        if (&origin != session) {
            QString text = QString("%1> %2").arg(origin.nickname(), message.right(message.length() - message.indexOf(':') - 1));
            // qDebug() << "text: " << text;
            session->sendMessage(text);
        }
    }
}

void UbiChatServer::handleSessionDisconection(UbiChatSession &session)
{
    // broadcast
    if (session.isRegistered()) {
        auto reply = QString("%1 left the chat, connection lost").arg(session.nickname());
        handleMessage(session, reply);
        qDebug() << reply;
        _sessions_registered.remove(session.nickname());
    } else {
        _sessions_pending.remove(&session);
        qDebug() << "unregistered session was removed";
    }

    session.deleteLater();
}
