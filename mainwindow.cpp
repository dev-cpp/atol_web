#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    _ui(new Ui::MainWindow)
{
    _ui->setupUi(this);
    this->setTrayIconActions();
    this->showTrayIcon();

    connect(_ui->installDriver,SIGNAL(triggered(bool)),this,SLOT(installDrivers()));
    connect(_ui->settingsKKT,SIGNAL(triggered(bool)),this,SLOT(settingsKKT()));
    connect(_ui->timeSync,SIGNAL(clicked(bool)),this,SLOT(timeSync()));

    _webserver = new WebServer();
    _thread = new QThread(this);
    _webserver->moveToThread(_thread);
    _thread->start();
    connect(this,SIGNAL(destroyed(QObject*)),_thread,SLOT(quit()));
    connect(_webserver,SIGNAL(destroyed(QObject*)),_webserver,SLOT(deleteLater()));
    connect(_webserver,SIGNAL(logSend(QString)),this,SLOT(logCreate(QString)));

    connect(_ui->reportX,SIGNAL(clicked(bool)),_webserver,SLOT(printXreport()));
    connect(_ui->reportZ,SIGNAL(clicked(bool)),_webserver,SLOT(printZreport()));
    connect(this,SIGNAL(reload()),_webserver,SLOT(reloadPrinter()));
    connect(_webserver,SIGNAL(lock()),this,SLOT(lockReport()));
    connect(_webserver,SIGNAL(unlock()),this,SLOT(unlockReport()));
    connect(this,SIGNAL(checkState()),_webserver,SLOT(chekServer()));
    connect(this,SIGNAL(timeSyncSignal(int,int,int)),_webserver,SLOT(timeSyncSlot(int,int,int)));
    emit checkState();
}

MainWindow::~MainWindow()
{
    QThread::currentThread()->destroyed();
    delete _ui;
}

//Слот установки драйвера устройства
void MainWindow::installDrivers()
{
    QProcess process(this);
    QString proc = QApplication::applicationDirPath() + "/Atol_USBCOM.exe";
    process.startDetached(proc);
}

//Слот настройки драйвера устройства
void MainWindow::settingsKKT()
{
    _settings = new Settings(this);
    _settings->show();
    connect(_settings,SIGNAL(reload()),this,SLOT(reloadPrinter()));
}

//Слот посылает сигнал пересоздания обьекта принтера
void MainWindow::reloadPrinter()
{
    emit reload();
    delete _settings;
}

//Слот блокирует кнопки отчетов
void MainWindow::lockReport()
{
    _ui->reportX->setDisabled(true);
    _ui->reportZ->setDisabled(true);
    _ui->timeSync->setDisabled(true);
}

//Слот разблокирует кнопки отчетов
void MainWindow::unlockReport()
{
    _ui->reportX->setEnabled(true);
    _ui->reportZ->setEnabled(true);
    _ui->timeSync->setEnabled(true);
}

//Слот завершает процесс EoU
void MainWindow::killEoU()
{
    QProcess killEoU(this);
    QString kill = QApplication::applicationDirPath() + "/EoU/kill.bat";
    killEoU.start(kill);
    killEoU.waitForFinished();
}

//Слот выводит информацию об ошибках
void MainWindow::logCreate(QString message)
{
    QIcon trayImage(":/images/icons-for-atol_10.png");
    _trayIcon->setIcon(trayImage);
    _log = new LogMessage();
    _log->show();
    connect(this,SIGNAL(logShow(QString)),_log,SLOT(message(QString)));
    emit logShow(message);
}

//Слот синхронизирует дату
void MainWindow::dateSync()
{
    QMessageBox msg("Синхронизация даты", "<h2>Синхронизировать дату на ККМ с ПК ?</h2>",
                    QMessageBox::Question,
                    QMessageBox::Yes | QMessageBox::Default,
                    QMessageBox::No | QMessageBox::Escape,
                    QMessageBox::NoButton);
    if(msg.exec() == QMessageBox::Yes)
    {
        QDate currDate = QDate::currentDate();
        int year = currDate.year();
        int month = currDate.month();
        int day = currDate.day();
        emit dateSyncSignal(day, month, year);
    }
}

//Слот синхронизирует время
void MainWindow::timeSync()
{
    QMessageBox msg("Синхронизация времени", "<h2>Синхронизировать время на ККМ с ПК ?</h2>",
                    QMessageBox::Question,
                    QMessageBox::Yes | QMessageBox::Default,
                    QMessageBox::No | QMessageBox::Escape,
                    QMessageBox::NoButton);
    if(msg.exec() == QMessageBox::Yes)
    {
        QTime currTime = QTime::currentTime();
        int hour = currTime.hour();
        int minute = currTime.minute();
        int second = currTime.second();
        emit timeSyncSignal(hour, minute, second);
    }
}

//Слот переопределяет кнопку сворачивания окна
void MainWindow::changeEvent(QEvent *event)
{
    QMainWindow::changeEvent(event);
    if(event->type() == QEvent::WindowStateChange)
    {
        if(isMinimized())
        {
            this->hide();
        }
    }
}

//Слот переопределяет кнопку закрытия окна
void MainWindow::closeEvent(QCloseEvent *event)
{
    this->hide();
    event->ignore();
}

//Слот сворачивает и разворачивает окно программы в трей
void MainWindow::trayIconActivated(QSystemTrayIcon::ActivationReason reason)
{
    switch(reason)
    {
        case QSystemTrayIcon::Trigger:
        case QSystemTrayIcon::DoubleClick:
            if(this->isVisible())
            {
                this->hide();
            }
            else{
                this->show();
            }
            break;
        default:
            break;
    }
}

//Меню программы в трее
void MainWindow::setTrayIconActions()
{
    _minimizeAction = new QAction("Свернуть", this);
    _restoreAction = new QAction("Восстановить", this);
    _quitAction = new QAction("Выход", this);

    connect(_minimizeAction,SIGNAL(triggered()),this,SLOT(hide()));
    connect(_restoreAction,SIGNAL(triggered()),this,SLOT(showNormal()));
    connect(_quitAction,SIGNAL(triggered()),qApp,SLOT(quit()));
    connect(_quitAction,SIGNAL(triggered()),this,SLOT(killEoU()));

    _trayIconMenu = new QMenu(this);
    _trayIconMenu->addAction(_minimizeAction);
    _trayIconMenu->addAction(_restoreAction);
    _trayIconMenu->addAction(_quitAction);
}

//Слот создает обьект трея
void MainWindow::showTrayIcon()
{
    _trayIcon = new QSystemTrayIcon(this);
    QIcon trayImage(":/images/icons-for-atol_07.png");
    _trayIcon->setIcon(trayImage);
    _trayIcon->setContextMenu(_trayIconMenu);

    connect(_trayIcon,SIGNAL(activated(QSystemTrayIcon::ActivationReason)),this,SLOT(trayIconActivated(QSystemTrayIcon::ActivationReason)));

    _trayIcon->show();
}
