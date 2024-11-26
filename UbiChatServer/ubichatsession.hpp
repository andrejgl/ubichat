#pragma once

#include <QObject>
#include <QTcpSocket>
#include <QTime>

class UbiChatSession : public QObject
{
    Q_OBJECT
public:
    explicit UbiChatSession(QTcpSocket *socket, QObject *parent = nullptr);
    ~UbiChatSession();

    void sendMessage(const QString &message);
    void registrate();

    const QString &nickname() const { return _nickname;  };
    qintptr id() const { return _net_socket->socketDescriptor(); };
    bool isRegistered() const { return _registred; }
    const QTime &activityTime() const { return _activityTime; }

signals:
    void messageReceived(UbiChatSession &session, const QString &message);
    void sessionDisconected(UbiChatSession &session);

private:
    void handleDisconnectFromClient();
    void readData();
    void handleError(QAbstractSocket::SocketError socketError);

    QString _nickname;
    QTcpSocket *_net_socket;
    QDataStream _in_data; // TODO: replace QDataStream with something more secure for data reading
    bool _registred = false;
    QTime _activityTime;
};
