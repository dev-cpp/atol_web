#ifndef WEBSERVER_H
#define WEBSERVER_H

#include <QThread>
#include <QString>
#include <QProcess>
#include <QTcpSocket>
#include <QTcpServer>
#include <QByteArray>
#include <QStringList>

#include "printer.h"

class WebServer : public QTcpServer
{
    Q_OBJECT

public:
    explicit WebServer(QObject *parent = nullptr);
    ~WebServer();
    void incomingConnection(qintptr handle);
    void createPrinter();    

signals:
    void printX();
    void printZ();
    void lock();
    void unlock();
    void checkState();
    void printReport(QStringList data);
    void logSend(QString log);
    void dateSyncSignal(int day, int month, int year);
    void timeSyncSignal(int hour, int minute, int second);

public slots:
    void printXreport();
    void printZreport();
    void reloadPrinter();
    void lockReport();
    void unlockReport();
    void chekServer();
    void logSendMessage(QString log);
    void dateSyncSlot(int day, int month, int year);
    void timeSyncSlot(int hour, int minute, int second);
    void onReadyRead();
    void onDisconnected();

private:
    QThread* _thread;
    Printer* _printer;
    QString _errorMessage;
};

#endif // WEBSERVER_H
