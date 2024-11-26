#include "ubichatconsole.h"

#include <QTextStream>
#include <QCoreApplication>
#include <iostream>
#include <QDebug>

UbiChatConsole::UbiChatConsole(QObject *parent)
    : QObject{parent}
    , _input_notif{new QSocketNotifier(fileno(stdin), QSocketNotifier::Read, this)}
    , _socket{nullptr}
{
    connect(_input_notif, &QSocketNotifier::activated, this, &UbiChatConsole::readInput);
}

UbiChatConsole::~UbiChatConsole()
{

    shutdown();
}

bool UbiChatConsole::connectToHost(const QString &address, uint16_t port, const QString &nickname)
{
    qDebug() << "try to open: " << address;
    if(_socket || address.isNull())
        return false;

    _nickname = nickname;
    QTcpSocket *new_socket = new QTcpSocket();

    connect(new_socket, &QIODevice::readyRead, this, &UbiChatConsole::readMessage);
    connect(new_socket, &QTcpSocket::errorOccurred, this, &UbiChatConsole::handleError);

    uint trial = 0;

    do {
        qDebug() << "try to connect";
        new_socket->connectToHost(address, port);
    } while (++trial < MAX_TRIALS && !new_socket->waitForConnected(trial > 1 ? TIMEOUT2 : TIMEOUT1));

    if (new_socket->state() != QTcpSocket::ConnectedState) {
        qDebug() << "Connection failed";
        delete new_socket;
        return false;
    }

    qDebug() << "Connected";

    new_socket->setParent(this);
    _in_data.setDevice(new_socket);
    _in_data.setVersion(QDataStream::Qt_6_5);
    connect(new_socket, &QTcpSocket::disconnected, this, &UbiChatConsole::handleDisconnectFromServer);

    _socket = new_socket;

    return true;
}

void UbiChatConsole::shutdown()
{
    if (_socket)
        _socket->disconnectFromHost();
}

void UbiChatConsole::readMessage()
{
    _in_data.startTransaction();

    QString message;
    _in_data >> message;

    if (!_in_data.commitTransaction())
        return;

    std::cout << message.toUtf8().constData() << std::endl;
}

void UbiChatConsole::handleError(QAbstractSocket::SocketError error)
{
    qDebug() << "socket error " << error;
}

void UbiChatConsole::handleDisconnectFromServer()
{
    qDebug() << "Server disconnected";
    QCoreApplication::exit(1);
}

void UbiChatConsole::readInput(QSocketDescriptor socket, QSocketNotifier::Type type)
{
    QTextStream in(stdin);
    QDataStream out(_socket);
    out.setVersion(QDataStream::Qt_6_5);

    out << QString("%1:%2").arg(_nickname, in.readLine());
}
