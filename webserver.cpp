#include "webserver.h"

WebServer::WebServer(QObject *parent) : QTcpServer(parent)
{
    if(listen(QHostAddress::Any, 9655))
    {
        createPrinter();
    }
    else
    {
        _errorMessage = "Web server: "+errorString();
    }    
}

WebServer::~WebServer()
{
}

//Метод обрабатывает входящее соединение от клиента
void WebServer::incomingConnection(qintptr handle)
{
    QTcpSocket* socket = new QTcpSocket();
    socket->setSocketDescriptor(handle);

    connect(socket,SIGNAL(readyRead()),this,SLOT(onReadyRead()));
    connect(socket,SIGNAL(disconnected()),this,SLOT(onDisconnected()));
}

//Метод создает обьект принтера
void WebServer::createPrinter()
{
    _printer = new Printer();
    _thread = new QThread(this);
    _printer->moveToThread(_thread);
    _thread->start();
    connect(this,SIGNAL(destroyed(QObject*)),_thread,SLOT(quit()));
    connect(_printer,SIGNAL(destroyed(QObject*)),_printer,SLOT(deleteLater()));
    connect(_printer,SIGNAL(logSend(QString)),this,SLOT(logSendMessage(QString)));

    connect(this,SIGNAL(printX()),_printer,SLOT(printXreport()));
    connect(this,SIGNAL(printZ()),_printer,SLOT(printZreport()));
    connect(this,SIGNAL(checkState()),_printer,SLOT(checkState()));
    connect(_printer,SIGNAL(lockReports()),this,SLOT(lockReport()));
    connect(_printer,SIGNAL(unlockReports()),this,SLOT(unlockReport()));
    connect(this,SIGNAL(printReport(QStringList)),_printer,SLOT(printReport(QStringList)));
    connect(this,SIGNAL(timeSyncSignal(int,int,int)),_printer,SLOT(timeSyncSlot(int,int,int)));
    emit checkState();
}

//Слот отправляет сигнал печати Х-отчета
void WebServer::printXreport()
{
    emit printX();
}

//Слот отправляет сигнал печати Z-отчета
void WebServer::printZreport()
{
    emit printZ();
}

//Слот пересоздает обьект принтера
void WebServer::reloadPrinter()
{
    emit _printer->destroyed();
    createPrinter();
}

//Слот посылает сигнал блокировки кнопок отчетов
void WebServer::lockReport()
{
    emit lock();
}

//Слот посылает сигнал разблокировки кнопок отчетов
void WebServer::unlockReport()
{
    emit unlock();
}

//Слот проверяетсостояние веб сервера
void WebServer::chekServer()
{
    if(!_errorMessage.isEmpty())
    {
        emit logSend(_errorMessage);
        _errorMessage.clear();
    }
}

//Слот синхронизации даты
void WebServer::dateSyncSlot(int day, int month, int year)
{
    emit dateSyncSignal(day, month, year);
}

//Слот синхронизации времени
void WebServer::timeSyncSlot(int hour, int minute, int second)
{
    emit timeSyncSignal(hour, minute, second);
}

//Слот отсылает сообщение об ошибке
void WebServer::logSendMessage(QString log)
{
    emit logSend(log);
}

//Слот читает информацию из сокета
void WebServer::onReadyRead()
{
    QTcpSocket* socket = qobject_cast<QTcpSocket*>(sender());
    QString data = socket->readAll();
    if(Printer::trigger != 1)
    {
        QByteArray response = "HTTP/1.1 200 OK\r\nContent-type: text/plain;charset=UTF-8\r\nAccess-Control-Allow-Origin: *\r\nConnection: close\r\n\r\n"
                              "{\"warning\": \"Настройте программу\"}\r\n";
        socket->write(response);
        socket->disconnectFromHost();
        return;
    }
    if(!data.contains("POST"))
    {
        QByteArray response = "HTTP/1.1 200 OK\r\nContent-type: text/plain;charset=UTF-8\r\nAccess-Control-Allow-Origin: *\r\nConnection: close\r\n\r\n";
        socket->write(response);
        socket->disconnectFromHost();
        return;
    }
    int start = data.indexOf('[');
    int end = data.indexOf(']') - 1;
    data = data.mid(start + 1, end - start);
    QStringList result = data.split(':');
    emit printReport(result);

    int timer = 0;
    int iter = 0;
    while(iter < 40)
    {
        QThread::msleep(500);
        if(!Printer::socketData.isEmpty())
        {
            QString buffer = "HTTP/1.1 200 OK\r\nContent-type: text/plain;charset=UTF-8\r\nAccess-Control-Allow-Origin: *\r\nConnection: close\r\n\r\n"+Printer::socketData+"\r\n";
            QByteArray response = buffer.toUtf8();
            socket->write(response);
            socket->disconnectFromHost();
            Printer::socketData.clear();
            return;
        }
        ++timer;
        if(timer >= 30)
        {
            socket->write("HTTP/1.1 200 OK\r\nContent-type: text/plain;charset=UTF-8\r\nAccess-Control-Allow-Origin: *\r\nConnection: close\r\n\r\n{\"error\": \"Disconnect timeout\"}\r\n");
            socket->disconnectFromHost();
            Printer::socketData.clear();
            timer = 0;
            return;
        }
        ++iter;
    }
    return;
}

//Слот закрывает сокетное соединение
void WebServer::onDisconnected()
{
    QTcpSocket* socket = qobject_cast<QTcpSocket*>(sender());
    socket->close();
    socket->deleteLater();
}
