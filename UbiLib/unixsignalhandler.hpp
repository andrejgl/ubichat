#pragma once

#include <QObject>
#include <QSocketNotifier>

class UnixSignalHandler : public QObject
{
    Q_OBJECT
public:
    explicit UnixSignalHandler(QObject *parent = nullptr);

    // Unix signal handlers.
    static void hupSignalHandler(int unused);
    static void termSignalHandler(int unused);
    static void IntSignalHandler(int unused);

signals:
    void hupSignalActivated();
    void termSignalActivated();
    void intSignalActivated();

private:
    QSocketNotifier *snHup;
    QSocketNotifier *snTerm;
    QSocketNotifier *snInt;

private slots:
    // Qt signal handlers.
    void handleSigHup();
    void handleSigTerm();
    void handleSigInt();
};
