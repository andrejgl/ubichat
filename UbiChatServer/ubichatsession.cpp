#include "ubichatsession.hpp"

UbiChatSession::UbiChatSession(QTcpSocket *socket, QObject *parent)
    : QObject{parent}
    , _net_socket{socket}
{
    _net_socket->setParent(this);

    _in_data.setDevice(_net_socket);
    _in_data.setVersion(QDataStream::Qt_6_5);

    _activityTime = QTime::currentTime();

    connect(_net_socket, &QTcpSocket::readyRead, this, &UbiChatSession::readData);
    connect(_net_socket,
            &QTcpSocket::disconnected,
            this,
            &UbiChatSession::handleDisconnectFromClient);
    connect(_net_socket, &QAbstractSocket::errorOccurred, this, &UbiChatSession::handleError);
}

UbiChatSession::~UbiChatSession()
{
    qDebug() << "session destroyed";
}

void UbiChatSession::sendMessage(const QString &message)
{
    Q_ASSERT(_net_socket);

    QByteArray block;
    QDataStream out(&block, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_6_5);

    out << message;
    _net_socket->write(block);
}

void UbiChatSession::registrate()
{
    _registred = true;
}

void UbiChatSession::handleDisconnectFromClient()
{
    qDebug() << "client disconnected";

    emit sessionDisconected(*this);
    _net_socket->disconnectFromHost();

}

void UbiChatSession::readData()
{
    _activityTime = QTime::currentTime();
    _in_data.startTransaction();

    QString message;
    _in_data >> message;

    if (!_in_data.commitTransaction())
        return;

    if (/*!_registred && */!_nickname.length()) {
        _nickname = message.left(message.indexOf(':'));
    }

    emit messageReceived(*this, message);
}

void UbiChatSession::handleError(QAbstractSocket::SocketError socketError)
{
    qDebug() << "connection error: " << socketError;
}
