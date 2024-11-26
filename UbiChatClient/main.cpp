#include <QCoreApplication>
#include <QCommandLineParser>
#include <iostream>
#include <signal.h>

#include "ubichatconsole.h"

static void sigquit(int sig)
{
    Q_UNUSED(sig);
    QCoreApplication::quit();
}

struct CommandLineParseResult
{
    enum class Status { Ok, Error, VersionRequested, HelpRequested };
    Status statusCode = Status::Ok;
    std::optional<QString> errorString = std::nullopt;
};

struct ConnectionParams
{
    QString serverAddress;
    quint16 serverPort;
    QString nickname;
};

CommandLineParseResult parseCommandLine(QCommandLineParser &parser, ConnectionParams &params)
{
    using Status = CommandLineParseResult::Status;

    QCommandLineOption serverAddressOption(
        QStringList() << "a" << "server-address",
        QCoreApplication::translate("main", "Server address to connect to <SERVER_ADDRESS>."),
        QCoreApplication::translate("main", "SERVER_ADDRESS"));
    parser.addOption(serverAddressOption);
    QCommandLineOption serverPortOption(
        QStringList() << "p" << "server-port",
        QCoreApplication::translate("main", "Server port to connect to <PORT_TO_CONNECT>."),
        QCoreApplication::translate("main", "PORT_TO_CONNECT"));
    parser.addOption(serverPortOption);
    QCommandLineOption nicknameOption(QStringList() << "n" << "nickname",
                                      QCoreApplication::translate("main", "Nickname to use."),
                                      QCoreApplication::translate("main", "NICKNAME"));
    parser.addOption(nicknameOption);

    const QCommandLineOption helpOption = parser.addHelpOption();
    const QCommandLineOption versionOption = parser.addVersionOption();

    if (!parser.parse(QCoreApplication::arguments()))
        return {Status::Error, parser.errorText()};

    if (parser.isSet(versionOption))
        return {Status::VersionRequested};

    if (parser.isSet(helpOption))
        return {Status::HelpRequested};

    if (parser.isSet(serverAddressOption)) {
        params.serverAddress = parser.value(serverAddressOption);
    } else {
        return {Status::Error, "Argument 'server-address' missing."};
    }

    if (parser.isSet(serverPortOption)) {
        params.serverPort = parser.value(serverPortOption).toInt();
    } else {
        return {Status::Error, "Argument 'server-port' missing."};
    }

    if (parser.isSet(nicknameOption)) {
        params.nickname = parser.value(nicknameOption);
    } else {
        return {Status::Error, "Argument 'nickname' missing."};
    }

    return { Status::Ok };
}

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);
    QCommandLineParser parser;
    parser.setApplicationDescription("UbiChat Client");

    ConnectionParams connectionParams;
    using Status = CommandLineParseResult::Status;
    CommandLineParseResult parseResult = parseCommandLine(parser, connectionParams);
    switch (parseResult.statusCode) {
    case Status::Ok:
        break;
    case Status::Error:
        std::cerr << qPrintable(parseResult.errorString.value_or("Unknown error occurred\n"));
        std::cerr << "\n\n";
        std::cerr << qPrintable(parser.helpText());
        return 1;
    case Status::VersionRequested:
        parser.showVersion();
        Q_UNREACHABLE_RETURN(0);
    case Status::HelpRequested:
        parser.showHelp();
        Q_UNREACHABLE_RETURN(0);
    }

    signal(SIGINT, sigquit);  // catch a CTRL-c signal

    UbiChatConsole console;
    QObject::connect(&a, &QCoreApplication::aboutToQuit, &console, &UbiChatConsole::shutdown);

    if (console.connectToHost(connectionParams.serverAddress, connectionParams.serverPort, connectionParams.nickname))
        return a.exec();

    return 1;
}
