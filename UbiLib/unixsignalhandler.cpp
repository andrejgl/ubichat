#include "unixsignalhandler.hpp"

#include <sys/socket.h>
#include <signal.h>

#include <QDebug>

static int sighupFd[2];
static int sigtermFd[2];
static int sigIntFd[2];

static int setup_unix_signal_handlers()
{
    struct sigaction hup, term;

    hup.sa_handler = UnixSignalHandler::hupSignalHandler;
    sigemptyset(&hup.sa_mask);
    hup.sa_flags = 0;
    hup.sa_flags |= SA_RESTART;

    if (sigaction(SIGHUP, &hup, 0))
        return 1;

    term.sa_handler = UnixSignalHandler::termSignalHandler;
    sigemptyset(&term.sa_mask);
    term.sa_flags = 0;
    term.sa_flags |= SA_RESTART;

    if (sigaction(SIGTERM, &term, 0))
        return 2;

    term.sa_handler = UnixSignalHandler::IntSignalHandler;
    sigemptyset(&term.sa_mask);
    term.sa_flags = 0;
    term.sa_flags |= SA_RESTART;

    if (sigaction(SIGINT, &term, 0))
        return 3;

    return 0;
}

UnixSignalHandler::UnixSignalHandler(QObject *parent)
    : QObject{parent}
{
    if (::socketpair(AF_UNIX, SOCK_STREAM, 0, sighupFd))
        qFatal("Couldn't create HUP socketpair");

    if (::socketpair(AF_UNIX, SOCK_STREAM, 0, sigtermFd))
        qFatal("Couldn't create TERM socketpair");

    if (::socketpair(AF_UNIX, SOCK_STREAM, 0, sigIntFd))
        qFatal("Couldn't create INT socketpair");

    snHup = new QSocketNotifier(sighupFd[1], QSocketNotifier::Read, this);
    connect(snHup, SIGNAL(activated(QSocketDescriptor)), this, SLOT(handleSigHup()));

    snTerm = new QSocketNotifier(sigtermFd[1], QSocketNotifier::Read, this);
    connect(snTerm, SIGNAL(activated(QSocketDescriptor)), this, SLOT(handleSigTerm()));

    snInt = new QSocketNotifier(sigIntFd[1], QSocketNotifier::Read, this);
    connect(snInt, SIGNAL(activated(QSocketDescriptor)), this, SLOT(handleSigInt()));
    // connect(snInt, &QSocketNotifier::activated, this, &UnixSignalHandler::handleSigInt);
}

void UnixSignalHandler::hupSignalHandler(int unused)
{
    char a = 1;
    ::write(sighupFd[0], &a, sizeof(a));
}

void UnixSignalHandler::termSignalHandler(int unused)
{
    char a = 1;
    ::write(sigtermFd[0], &a, sizeof(a));
}

void UnixSignalHandler::IntSignalHandler(int unused)
{
    char a = 1;
    ::write(sigIntFd[0], &a, sizeof(a));
}

void UnixSignalHandler::handleSigHup()
{
    snHup->setEnabled(false);
    char tmp;
    ::read(sighupFd[1], &tmp, sizeof(tmp));

    // do Qt stuff
    emit hupSignalActivated();

    snHup->setEnabled(true);
}

void UnixSignalHandler::handleSigTerm()
{
    snTerm->setEnabled(false);
    char tmp;
    ::read(sigtermFd[1], &tmp, sizeof(tmp));

    // do Qt stuff
    emit termSignalActivated();

    snTerm->setEnabled(true);
}

void UnixSignalHandler::handleSigInt()
{
    qDebug() << "UnixSignalHandler::handleSigInt()";

    snTerm->setEnabled(false);
    char tmp;
    ::read(sigIntFd[1], &tmp, sizeof(tmp));

    // do Qt stuff
    emit intSignalActivated();

    snTerm->setEnabled(true);
}
