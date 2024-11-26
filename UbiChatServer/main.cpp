#include <QCoreApplication>
#include <QCommandLineParser>
#include <iostream>
#include <signal.h>

#include "ubichatserver.hpp"

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

struct ServerParams
{
    quint16 listenPort;
};

CommandLineParseResult parseCommandLine(QCommandLineParser &parser, ServerParams &params)
{
    using Status = CommandLineParseResult::Status;

    QCommandLineOption listenPortOption(
        QStringList() << "p" << "listen-port",
        QCoreApplication::translate("main", "Port number to listen <PORT_NUMBER_TO_LISTEN>."),
        QCoreApplication::translate("main", "PORT_NUMBER_TO_LISTEN"));
    parser.addOption(listenPortOption);

    const QCommandLineOption helpOption = parser.addHelpOption();
    const QCommandLineOption versionOption = parser.addVersionOption();

    if (!parser.parse(QCoreApplication::arguments()))
        return {Status::Error, parser.errorText()};

    if (parser.isSet(versionOption))
        return {Status::VersionRequested};

    if (parser.isSet(helpOption))
        return {Status::HelpRequested};

    if (parser.isSet(listenPortOption)) {
        params.listenPort = parser.value(listenPortOption).toInt();
    } else {
        return {Status::Error, "Argument 'listen-port' missing."};
    }

    return { Status::Ok };
}

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);
    QCommandLineParser parser;
    parser.setApplicationDescription("UbiChat Server");

    ServerParams serverParams;
    using Status = CommandLineParseResult::Status;
    CommandLineParseResult parseResult = parseCommandLine(parser, serverParams);
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

    UbiChatServer server;
    QObject::connect(&a, &QCoreApplication::aboutToQuit, &server, &UbiChatServer::shutdown);

    if (server.start(serverParams.listenPort))
        return a.exec();

    return 1;
}
