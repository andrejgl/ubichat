#pragma once

#include <QObject>
#include <QSocketNotifier>
#include <QTcpSocket>

class UbiChatConsole : public QObject
{
    Q_OBJECT
public:
    explicit UbiChatConsole(QObject *parent = nullptr);
    virtual ~UbiChatConsole();

    bool connectToHost(const QString &address, uint16_t port, const QString &nickname);
    void shutdown();

signals:

private:
    static constexpr uint MAX_TRIALS = 5;
    static constexpr uint TIMEOUT1 = 3000;
    static constexpr uint TIMEOUT2 = 5000;

    void readMessage();
    void handleError(QAbstractSocket::SocketError);
    void handleDisconnectFromServer();

    QSocketNotifier *_input_notif;
    QTcpSocket      *_socket;
    QDataStream     _in_data;
    QString _nickname;

private slots:
    void readInput(QSocketDescriptor socket, QSocketNotifier::Type type);
};
