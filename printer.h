#ifndef PRINTER_H
#define PRINTER_H

#include <QDir>
#include <QFile>
#include <QChar>
#include <QObject>
#include <QString>
#include <QThread>
#include <QProcess>
#include <QStringList>
#include <QMessageBox>
#include <QTextStream>
#include <QApplication>

#include "eou.h"
#include "logmessage.h"
#include "include/ifptr.h"
#include "include/dto_const.h"
#include "include/dto_errors.h"

class Printer : public QObject
{
    Q_OBJECT
public:
    explicit Printer(QObject *parent = nullptr);
    ~Printer();
    void openSession(QStringList data);
    void setupPrinter(QStringList data);
    void sellReport(QStringList data);
    void winReport(QStringList data);
    void returnReport(QStringList data);
    void returnWinReport(QStringList data);
    void sellCorrection(QStringList data);
    void winCorrection(QStringList data);
    void incomeReport(QStringList data);
    void outcomeReport(QStringList data);
    void encashment(QStringList data);
    void cashReport();
    void printReportX();
    void printReportZ();
    void departmentReport();
    int printBarcode(TED::Fptr::BarcodeType type, const wchar_t *barcode, double scale = 100);
    int openCheck(TED::Fptr::ChequeType type);
    int closeCheck(int typeClose);
    int registrationFZ54(const wchar_t *name, double price, double quantity, double positionSum, int taxNumber, int department);
    int payment(double sum, int type);
    QString checkError();
    static QString socketData;
    static int trigger;

signals:
    void lockReports();
    void unlockReports();
    void createEoU();
    void logSend(QString log);

public slots:
    void printXreport();
    void printZreport();
    void checkState();
    void printReport(QStringList data);
    void dateSyncSlot(int day, int month, int year);
    void timeSyncSlot(int hour, int minute, int second);

private:
    TED::Fptr::IFptr* _ifptr;
    QThread* _thread;
    EoU* _eou;
    QString _logMessage;
    int _state;
    int _printerModel;
    QString _errorMessage;
};

#endif // PRINTER_H
