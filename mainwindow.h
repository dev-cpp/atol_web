#ifndef MAINWINDOW_H
#define MAINWINDOW_H


#include <QDate>
#include <QTime>
#include <QThread>
#include <QString>
#include <QProcess>
#include <QMainWindow>
#include <QCloseEvent>
#include <QMessageBox>
#include <QApplication>
#include <QSystemTrayIcon>

#include "settings.h"
#include "webserver.h"
#include "logmessage.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

signals:
    void reload();
    void checkState();
    void logShow(QString log);
    void dateSyncSignal(int day, int month, int year);
    void timeSyncSignal(int hour, int minute, int second);

public slots:
    void installDrivers();
    void settingsKKT();
    void reloadPrinter();
    void lockReport();
    void unlockReport();
    void killEoU();
    void logCreate(QString message);
    void dateSync();
    void timeSync();

private slots:
    void changeEvent(QEvent *event);
    void closeEvent(QCloseEvent *event);
    void trayIconActivated(QSystemTrayIcon::ActivationReason reason);
    void setTrayIconActions();
    void showTrayIcon();

private:
    Ui::MainWindow* _ui;
    QThread* _thread;
    WebServer* _webserver;
    Settings* _settings;
    LogMessage* _log;

    QMenu* _trayIconMenu;
    QAction* _minimizeAction;
    QAction* _restoreAction;
    QAction* _quitAction;
    QSystemTrayIcon* _trayIcon;
};

#endif // MAINWINDOW_H
