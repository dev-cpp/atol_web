#include "printer.h"

Printer::Printer(QObject *parent) : QObject(parent)
{
    QProcess killEoU(this);
    QString kill = QApplication::applicationDirPath() + "/EoU/kill.bat";
    killEoU.start(kill);
    killEoU.waitForFinished();

    _state = 0;
    _printerModel = 0;
    QStringList buffer;
    QFile settingsFile(QApplication::applicationDirPath()+"/settings.ini");
    if(!settingsFile.open(QIODevice::ReadOnly))
    {
        settingsFile.open(QIODevice::WriteOnly);
        settingsFile.close();
        if(!settingsFile.open(QIODevice::ReadOnly))
            _errorMessage = "Can't open settings file!!!";
        return;
    }
    while(!settingsFile.atEnd())
    {
        buffer.append(settingsFile.readLine().replace("\r\n", ""));
    }
    if(!buffer.isEmpty())
    {
        trigger = 1;
        QString num = buffer[1];
        int port = num.toInt();
        _ifptr = CreateFptrInterface(DTO_IFPTR_VER1);
        if(!_ifptr)
        {
            _errorMessage = "Can't create driver object!!!";
            return;
        }
        _ifptr->put_DeviceSingleSetting(S_PORT, port);
        _ifptr->put_DeviceSingleSetting(S_VID, 0x2912);
        _ifptr->put_DeviceSingleSetting(S_PID, 0x0005);
        _ifptr->put_DeviceSingleSetting(S_PROTOCOL, TED::Fptr::ProtocolAtol30);
        if(buffer[0] == "2")
        {
            _printerModel = 30;
            _ifptr->put_DeviceSingleSetting(S_MODEL, TED::Fptr::ModelATOL30F);
        }
        else if(buffer[0] == "1")
        {
            _printerModel = 22;
            _ifptr->put_DeviceSingleSetting(S_MODEL, TED::Fptr::ModelATOL22F);
        }
        _ifptr->put_DeviceSingleSetting(S_BAUDRATE, 115200);
        _ifptr->ApplySingleSettings();
        if(_ifptr->put_DeviceEnabled(1) < 0)
        {
            _errorMessage = checkError();
            return;
        }
        _state = 1;

        QFile settingsEoU(QApplication::applicationDirPath()+"/EoU/settings.xml");
        QString settings = "<?xml version=\"1.0\" encoding=\"windows-1251\" ?>\r\n<settings>\r\n    <logs>\r\n        <dir>"+QApplication::applicationDirPath()+"/EoU/logs</dir>\r\n"
                +"    </logs>\r\n    <device>\r\n        <id>Atol</id>\r\n        <port>"+buffer[2]+"</port>\r\n    </device>\r\n</settings>";
        if(!settingsEoU.open(QIODevice::WriteOnly | QIODevice::Text))
            _errorMessage = "Can't open EoU settings file!!!";
        QTextStream stream(&settingsEoU);
        stream << settings;
        settingsEoU.flush();
        settingsEoU.close();

        _thread = new QThread(this);
        _eou = new EoU();
        _eou->moveToThread(_thread);
        _thread->start();
        connect(this,SIGNAL(createEoU()),_eou,SLOT(createEoU()));
        connect(this,SIGNAL(destroyed(QObject*)),_thread,SLOT(quit()));
        connect(_eou,SIGNAL(destroyed(QObject*)),_eou,SLOT(deleteLater()));
        emit createEoU();
    }    
}

QString Printer::socketData;
int Printer::trigger = 0;

Printer::~Printer()
{
    if(_state == 1)
        ReleaseFptrInterface(&_ifptr);
}

//Метод открывает рабочую смену
void Printer::openSession(QStringList data)
{
    if(data.length() != 2)
    {
        socketData = "{\"error\": \"Неверный формат данных\"}";
        return;
    }

    _ifptr->put_UserPassword(L"29");
    _ifptr->put_Mode(TED::Fptr::ModeRegistration);
    _ifptr->SetMode();
    _ifptr->CancelCheck();

    QString buffer = data[1];
    wchar_t* FIO = new wchar_t[buffer.length()+1];
    buffer.toWCharArray(FIO);
    FIO[buffer.length()] = 0;
    _ifptr->put_UserPassword(L"30");
    _ifptr->put_Mode(TED::Fptr::ModeProgramming);
    _ifptr->SetMode();
    _ifptr->put_Caption(FIO);
    _ifptr->put_CaptionPurpose(117);
    if(_ifptr->SetCaption() < 0)
    {
        socketData = "{\"error\": \""+checkError()+"\"}";
        return;
    }

    _ifptr->ResetMode();
    _ifptr->put_UserPassword(L"29");
    _ifptr->put_Mode(TED::Fptr::ModeRegistration);
    _ifptr->SetMode();
    if(_ifptr->OpenSession() < 0)
    {
        socketData = "{\"error\": \""+checkError()+"\"}";
        return;
    }

    delete [] FIO;

    socketData = "{\"noerror\": \"OK\"}";
}

//Метод первончальной настройки принтера
void Printer::setupPrinter(QStringList data)
{
    if(data.length() != 2)
    {
        socketData = "{\"error\": \"Неверный формат данных\"}";
        return;
    }

    _ifptr->put_UserPassword(L"29");
    _ifptr->put_Mode(TED::Fptr::ModeRegistration);
    _ifptr->SetMode();
    _ifptr->CancelCheck();

    QString buffer = data[1];
    int count = 0;
    if(_printerModel == 30)
    {
        if(buffer.length() >= 16 && buffer.length() < 32)
        {
            count = (32 - buffer.length())/2;
            for(int i = 0; i < count; ++i)
            {
                buffer.prepend(" ");
            }
        }
        else if(buffer.length() < 16)
        {
            for(int i = 0; i < buffer.length(); i += 2)
            {
                buffer.insert(i, "\u0009");
            }
            count = (32 - buffer.length())/2;
            for(int i = 0; i < count; ++i)
            {
                buffer.prepend(" ");
            }
        }
    }
    else if(_printerModel == 22)
    {
        if(buffer.length() >= 24 && buffer.length() < 48)
        {
            count = (48 - buffer.length())/2;
            for(int i = 0; i < count; ++i)
            {
                buffer.prepend(" ");
            }
        }
        else if(buffer.length() < 24)
        {
            for(int i = 0; i < buffer.length(); i += 2)
            {
                buffer.insert(i, "\u0009");
            }
            count = (48 - buffer.length())/2;
            for(int i = 0; i < count; ++i)
            {
                buffer.prepend(" ");
            }
        }
    }
    wchar_t* Firm = new wchar_t[buffer.length()+1];
    buffer.toWCharArray(Firm);
    Firm[buffer.length()] = 0;
    _ifptr->put_UserPassword(L"30");
    _ifptr->put_Mode(TED::Fptr::ModeProgramming);
    _ifptr->SetMode();
    _ifptr->put_Caption(Firm);
    _ifptr->put_CaptionPurpose(74);
    if(_ifptr->SetCaption() < 0)
    {
        socketData = "{\"error\": \""+checkError()+"\"}";
        return;
    }
    _ifptr->put_Caption(L"ПЛАТ.КАРТОЙ");
    _ifptr->put_CaptionPurpose(66);
    if(_ifptr->SetCaption() < 0)
    {
        socketData = "{\"error\": \""+checkError()+"\"}";
        return;
    }

    _ifptr->put_Value(1);
    _ifptr->put_ValuePurpose(39);
    if(_ifptr->SetValue() < 0)
    {
        socketData = "{\"error\": \""+checkError()+"\"}";
        return;
    }
    _ifptr->put_Value(1);
    _ifptr->put_ValuePurpose(72);
    if(_ifptr->SetValue() < 0)
    {
        socketData = "{\"error\": \""+checkError()+"\"}";
        return;
    }
    _ifptr->put_Value(8);
    _ifptr->put_ValuePurpose(66);
    if(_ifptr->SetValue() < 0)
    {
        socketData = "{\"error\": \""+checkError()+"\"}";
        return;
    }
    _ifptr->put_Value(4);
    _ifptr->put_ValuePurpose(259);
    if(_ifptr->SetValue() < 0)
    {
        socketData = "{\"error\": \""+checkError()+"\"}";
        return;
    }

    delete [] Firm;

    socketData = "{\"noerror\": \"OK\"}";
}

//Метод регистрирует продажу
void Printer::sellReport(QStringList data)
{
    if(data.length() != 7)
    {
        socketData = "{\"error\": \"Неверный формат данных\"}";
        return;
    }

    _ifptr->put_UserPassword(L"29");
    _ifptr->put_Mode(TED::Fptr::ModeRegistration);
    _ifptr->SetMode();
    _ifptr->CancelCheck();

    QString name = data[1];
    QString price = data[2];
    QString sum = data[3];
    QString type = data[4];
    QString dep = data[5];
    QString barcode = data[6];
    wchar_t* positionName = new wchar_t[name.length()+1];
    name.toWCharArray(positionName);
    positionName[name.length()] = 0;
    double positionPrice = price.toDouble();
    double positionSum = sum.toDouble();
    int typeClose = type.toInt();
    if(typeClose < 0 || typeClose > 1)
    {
        socketData = "{\"error\": \"Неверное значение\"}";
        return;
    }
    int department = dep.toInt();
    if(department < 0 || department > 30)
    {
        socketData = "{\"error\": \"Неверное значение\"}";
        return;
    }

    if(positionPrice > positionSum)
    {
        socketData = "{\"error\": \"Сумма ставки больше суммы оплаты\"}";
        return;
    }

    wchar_t* barcodeName = new wchar_t[barcode.length()+1];
    barcode.toWCharArray(barcodeName);
    barcodeName[barcode.length()] = 0;
    if(_printerModel == 22)
    {
        if(printBarcode(TED::Fptr::BarcodeCode39, barcodeName) < 0)
            return;
    }
    else if(_printerModel == 30)
    {
        if(printBarcode(TED::Fptr::BarcodeCode39, barcodeName, 90) < 0)
            return;
    }
    _ifptr->put_Caption(barcodeName);
    _ifptr->put_TextWrap(TED::Fptr::TextWrapWord);
    _ifptr->put_Alignment(TED::Fptr::AlignmentCenter);
    _ifptr->PrintString();
    _ifptr->put_Caption(L" ");
    _ifptr->PrintString();

    if(openCheck(TED::Fptr::ChequeSell) < 0)
        return;
    if(registrationFZ54(positionName, positionPrice, 1, positionPrice, TED::Fptr::TaxVATNo, department) < 0)
        return;
    if(payment(positionSum, typeClose) < 0)
        return;
    if(closeCheck(typeClose) < 0)
        return;
    _ifptr->put_Summ(0);

    delete [] positionName;
    delete [] barcodeName;

    socketData = "{\"noerror\": \"OK\"}";
}

//Метод регистрирует выигрыш
void Printer::winReport(QStringList data)
{
    if(data.length() != 4)
    {
        socketData = "{\"error\": \"Неверный формат данных\"}";
        return;
    }

    QString name = data[1];
    QString price = data[2];
    QString dep = data[3];
    wchar_t* positionName = new wchar_t[name.length()+1];
    name.toWCharArray(positionName);
    positionName[name.length()] = 0;
    double positionPrice = price.toDouble();
    int department = dep.toInt();
    if(department < 0 || department > 30)
    {
        socketData = "{\"error\": \"Неверное значение\"}";
        return;
    }

    double cashSumm = 0.0;
    _ifptr->GetSumm();
    _ifptr->get_Summ(&cashSumm);
    _ifptr->put_Summ(0);
    if(cashSumm < positionPrice)
    {
        socketData = "{\"error\": \"Недостаточно наличных в кассе\"}";
        return;
    }

    _ifptr->put_UserPassword(L"29");
    _ifptr->put_Mode(TED::Fptr::ModeRegistration);
    _ifptr->SetMode();
    _ifptr->CancelCheck();

    if(openCheck(TED::Fptr::ChequeBuy) < 0)
        return;
    _ifptr->put_PositionSum(positionPrice);
    _ifptr->put_Quantity(1);
    _ifptr->put_Price(positionPrice);
    _ifptr->put_TaxNumber(TED::Fptr::TaxVATNo);
    _ifptr->put_TextWrap(TED::Fptr::TextWrapWord);
    _ifptr->put_Name(positionName);
    _ifptr->put_Department(department);
    if(_ifptr->Buy() < 0)
    {
        socketData = "{\"error\": \""+checkError()+"\"}";
        return;
    }
    if(payment(positionPrice, 0) < 0)
        return;
    if(closeCheck(0) < 0)
        return;
    _ifptr->put_Summ(0);

    delete [] positionName;

    socketData = "{\"noerror\": \"OK\"}";
}

//Метод регистрирует возврат
void Printer::returnReport(QStringList data)
{
    if(data.length() != 4)
    {
        socketData = "{\"error\": \"Неверный формат данных\"}";
        return;
    }

    QString name = data[1];
    QString price = data[2];
    QString dep = data[3];
    int department = dep.toInt();
    if(department < 0 || department > 30)
    {
        socketData = "{\"error\": \"Неверное значение\"}";
        return;
    }
    wchar_t* positionName = new wchar_t[name.length()+1];
    name.toWCharArray(positionName);
    positionName[name.length()] = 0;
    double positionPrice = price.toDouble();

    double cashSumm = 0.0;
    _ifptr->GetSumm();
    _ifptr->get_Summ(&cashSumm);
    _ifptr->put_Summ(0);
    if(cashSumm < positionPrice)
    {
        socketData = "{\"error\": \"Недостаточно наличных в кассе\"}";
        return;
    }

    _ifptr->put_UserPassword(L"29");
    _ifptr->put_Mode(TED::Fptr::ModeRegistration);
    _ifptr->SetMode();
    _ifptr->CancelCheck();

    if(openCheck(TED::Fptr::ChequeSellReturn) < 0)
        return;
    _ifptr->put_PositionSum(positionPrice);
    _ifptr->put_Quantity(1);
    _ifptr->put_Price(positionPrice);
    _ifptr->put_TaxNumber(TED::Fptr::TaxVATNo);
    _ifptr->put_TextWrap(TED::Fptr::TextWrapWord);
    _ifptr->put_Name(positionName);
    _ifptr->put_EnableCheckSumm(1);
    _ifptr->put_Department(department);
    if(_ifptr->Return() < 0)
    {
        socketData = "{\"error\": \""+checkError()+"\"}";
        return;
    }
    if(payment(positionPrice, 0) < 0)
        return;
    if(closeCheck(0) < 0)
        return;
    _ifptr->put_Summ(0);

    delete [] positionName;

    socketData = "{\"noerror\": \"OK\"}";
}

//Метод регистрирует вовзврат выигрыша
void Printer::returnWinReport(QStringList data)
{
    if(data.length() != 4)
    {
        socketData = "{\"error\": \"Неверный формат данных\"}";
        return;
    }

    _ifptr->put_UserPassword(L"29");
    _ifptr->put_Mode(TED::Fptr::ModeRegistration);
    _ifptr->SetMode();
    _ifptr->CancelCheck();

    if(openCheck(TED::Fptr::ChequeBuyReturn) < 0)
        return;
    QString name = data[1];
    QString price = data[2];
    QString dep = data[3];
    wchar_t* positionName = new wchar_t[name.length()+1];
    name.toWCharArray(positionName);
    positionName[name.length()] = 0;
    double positionPrice = price.toDouble();
    int department = dep.toInt();
    if(department < 0 || department > 30)
    {
        socketData = "{\"error\": \"Неверное значение\"}";
        return;
    }

    _ifptr->put_PositionSum(positionPrice);
    _ifptr->put_Quantity(1);
    _ifptr->put_Price(positionPrice);
    _ifptr->put_TaxNumber(TED::Fptr::TaxVATNo);
    _ifptr->put_TextWrap(TED::Fptr::TextWrapWord);
    _ifptr->put_Name(positionName);
    _ifptr->put_Department(department);
    if(_ifptr->BuyReturn() < 0)
    {
        socketData = "{\"error\": \""+checkError()+"\"}";
        return;
    }
    if(payment(positionPrice, 0) < 0)
        return;
    if(closeCheck(0) < 0)
        return;
    _ifptr->put_Summ(0);

    delete [] positionName;

    socketData = "{\"noerror\": \"OK\"}";
}

//Метод регистрирует коррекцию прихода
void Printer::sellCorrection(QStringList data)
{
    if(data.length() != 2)
    {
        socketData = "{\"error\": \"Неверный формат данных\"}";
        return;
    }

    _ifptr->put_UserPassword(L"29");
    _ifptr->put_Mode(TED::Fptr::ModeRegistration);
    _ifptr->SetMode();
    _ifptr->CancelCheck();

    if(openCheck(TED::Fptr::ChequeSellCorrection) < 0)
        return;
    QString price = data[1];
    double positionPrice = price.toDouble();

    _ifptr->put_PositionSum(positionPrice);
    _ifptr->put_Quantity(1);
    _ifptr->put_Price(positionPrice);
    _ifptr->put_TaxNumber(TED::Fptr::TaxVATNo);
    _ifptr->put_TextWrap(TED::Fptr::TextWrapWord);
    if(_ifptr->Correction() < 0)
    {
        socketData = "{\"error\": \""+checkError()+"\"}";
        return;
    }
    if(payment(positionPrice, 0) < 0)
        return;
    if(closeCheck(0) < 0)
        return;
    _ifptr->put_Summ(0);

    socketData = "{\"noerror\": \"OK\"}";
}

//Метод регистрирует коррекцию расхода
void Printer::winCorrection(QStringList data)
{
    if(data.length() != 2)
    {
        socketData = "{\"error\": \"Неверный формат данных\"}";
        return;
    }

    _ifptr->put_UserPassword(L"29");
    _ifptr->put_Mode(TED::Fptr::ModeRegistration);
    _ifptr->SetMode();
    _ifptr->CancelCheck();

    if(openCheck(TED::Fptr::ChequeBuyCorrection) < 0)
        return;
    QString price = data[1];
    double positionPrice = price.toDouble();

    _ifptr->put_PositionSum(positionPrice);
    _ifptr->put_Quantity(1);
    _ifptr->put_Price(positionPrice);
    _ifptr->put_TaxNumber(TED::Fptr::TaxVATNo);
    _ifptr->put_TextWrap(TED::Fptr::TextWrapWord);
    if(_ifptr->BuyCorrection() < 0)
    {
        socketData = "{\"error\": \""+checkError()+"\"}";
        return;
    }
    if(payment(positionPrice, 0) < 0)
        return;
    if(closeCheck(0) < 0)
        return;
    _ifptr->put_Summ(0);

    socketData = "{\"noerror\": \"OK\"}";
}

//Метод регистрирует внесение
void Printer::incomeReport(QStringList data)
{
    if(data.length() != 2)
    {
        socketData = "{\"error\": \"Неверный формат данных\"}";
        return;
    }

    QString sum = data[1];

    _ifptr->put_UserPassword(L"29");
    _ifptr->put_Mode(TED::Fptr::ModeRegistration);
    _ifptr->SetMode();
    _ifptr->CancelCheck();

    _ifptr->put_UserPassword(L"29");
    _ifptr->put_Mode(TED::Fptr::ModeRegistration);
    _ifptr->SetMode();
    double positionSum = sum.toDouble();
    _ifptr->put_Summ(positionSum);
    if(_ifptr->CashIncome() < 0)
    {
        socketData = "{\"error\": \""+checkError()+"\"}";
        return;
    }
    _ifptr->put_Summ(0);

    socketData = "{\"noerror\": \"OK\"}";
}

//Метод регистрирует изьятие
void Printer::outcomeReport(QStringList data)
{
    if(data.length() != 2)
    {
        socketData = "{\"error\": \"Неверный формат данных\"}";
        return;
    }

    QString sum = data[1];
    double positionSum = sum.toDouble();

    _ifptr->put_UserPassword(L"29");
    _ifptr->put_Mode(TED::Fptr::ModeRegistration);
    _ifptr->SetMode();
    _ifptr->CancelCheck();

    double cashSumm = 0.0;
    _ifptr->GetSumm();
    if(_ifptr->get_Summ(&cashSumm) < 0)
    {
        socketData = "{\"error\": \""+checkError()+"\"}";
        return;
    }
    _ifptr->put_Summ(0);
    if(cashSumm < positionSum)
    {
        socketData = "{\"error\": \"Недостаточно наличных в кассе\"}";
        return;
    }
    _ifptr->put_UserPassword(L"29");
    _ifptr->put_Mode(TED::Fptr::ModeRegistration);
    _ifptr->SetMode();
    _ifptr->put_Summ(positionSum);
    if(_ifptr->CashOutcome() < 0)
    {
        socketData = "{\"error\": \""+checkError()+"\"}";
        return;
    }
    _ifptr->put_Summ(0);

    socketData = "{\"noerror\": \"OK\"}";
}

//Метод устанавливает или снимает признак инкасации
void Printer::encashment(QStringList data)
{
    if(data.length() != 2)
    {
        socketData = "{\"error\": \"Неверный формат данных\"}";
        return;
    }
    QString val = data[1];

    _ifptr->put_UserPassword(L"29");
    _ifptr->put_Mode(TED::Fptr::ModeRegistration);
    _ifptr->SetMode();
    _ifptr->CancelCheck();

    _ifptr->put_UserPassword(L"30");
    _ifptr->put_Mode(TED::Fptr::ModeProgramming);
    _ifptr->SetMode();
    int value = 0;
    value = val.toInt();
    if(value < 0 || value > 1)
    {
        socketData = "{\"error\": \"Неверное значение\"}";
        return;
    }
    _ifptr->put_Value(value);
    _ifptr->put_ValuePurpose(55);
    if(_ifptr->SetValue() < 0)
    {
        socketData = "{\"error\": \""+checkError()+"\"}";
        return;
    }

    socketData = "{\"noerror\": \"OK\"}";
}

//Метод возвращает кол-во денег в денежном ящике
void Printer::cashReport()
{
    _ifptr->put_UserPassword(L"29");
    _ifptr->put_Mode(TED::Fptr::ModeRegistration);
    _ifptr->SetMode();
    _ifptr->CancelCheck();

    double cashSumm = 0.0;
    _ifptr->GetSumm();
    if(_ifptr->get_Summ(&cashSumm) < 0)
    {
        socketData = "{\"error\": \""+checkError()+"\"}";
        return;
    }
    _ifptr->put_Summ(0);
    socketData = "{\"cash\": \""+QString::number(cashSumm)+"\"}";
}

//Метод печатает Х отчет
void Printer::printReportX()
{
    _ifptr->put_UserPassword(L"29");
    _ifptr->put_Mode(TED::Fptr::ModeRegistration);
    _ifptr->SetMode();
    _ifptr->CancelCheck();

    _ifptr->put_UserPassword(L"29");
    _ifptr->put_Mode(TED::Fptr::ModeReportNoClear);
    _ifptr->SetMode();
    _ifptr->put_ReportType(TED::Fptr::ReportX);
    if(_ifptr->Report() < 0)
    {
        socketData = "{\"error\": \""+checkError()+"\"}";
        return;
    }

    socketData = "{\"noerror\": \"OK\"}";
}

//Метод печатает Z отчет
void Printer::printReportZ()
{
    _ifptr->put_UserPassword(L"29");
    _ifptr->put_Mode(TED::Fptr::ModeRegistration);
    _ifptr->SetMode();
    _ifptr->CancelCheck();

    _ifptr->put_UserPassword(L"29");
    _ifptr->put_Mode(TED::Fptr::ModeReportClear);
    _ifptr->SetMode();
    _ifptr->put_ReportType(TED::Fptr::ReportZ);
    if(_ifptr->Report() < 0)
    {
        socketData = "{\"error\": \""+checkError()+"\"}";
        return;
    }

    socketData = "{\"noerror\": \"OK\"}";
}

//Метод формирует отчет по секциям
void Printer::departmentReport()
{
    _ifptr->put_UserPassword(L"29");
    _ifptr->put_Mode(TED::Fptr::ModeRegistration);
    _ifptr->SetMode();
    _ifptr->CancelCheck();

    _ifptr->put_UserPassword(L"29");
    _ifptr->put_Mode(TED::Fptr::ModeReportNoClear);
    _ifptr->SetMode();
    _ifptr->put_ReportType(TED::Fptr::ReportDepartment);
    if(_ifptr->Report() < 0)
    {
        socketData = "{\"error\": \""+checkError()+"\"}";
        return;
    }

    socketData = "{\"noerror\": \"OK\"}";
}

//Слот печатает Х-отчет
void Printer::printXreport()
{
    printReportX();
}

//Слот печатает Z-отчет
void Printer::printZreport()
{
    printReportZ();
}

//Слот проверяет состояние обьекта принтера
void Printer::checkState()
{
    if(_state == 0)
        emit lockReports();
    if(_state == 1)
        emit unlockReports();
    if(!_errorMessage.isEmpty())
    {
        emit logSend(_errorMessage);
        _errorMessage.clear();
    }
}

//Слот формирует чеки и печатает их на принтере
void Printer::printReport(QStringList data)
{
    if(data[0] == "sellreport"){
        sellReport(data);
        return;
    }
    else if(data[0] == "sellreturn"){
        returnReport(data);
        return;
    }
    else if(data[0] == "winreport"){
        winReport(data);
        return;
    }
    else if(data[0] == "winreturn"){
        returnWinReport(data);
        return;
    }
    else if(data[0] == "income"){
        incomeReport(data);
        return;
    }
    else if(data[0] == "open"){
        openSession(data);
        return;
    }
    else if(data[0] == "xreport"){
        printReportX();
        return;
    }
    else if(data[0] == "department"){
        departmentReport();
        return;
    }
    else if(data[0] == "outcome"){
        outcomeReport(data);
        return;
    }
    else if(data[0] == "zreport"){
        printReportZ();
        return;
    }    
    else if(data[0] == "setup"){
        setupPrinter(data);
        return;
    }    
    else if(data[0] == "cashreport"){
        cashReport();
        return;
    }
    else if(data[0] == "encashment"){
        encashment(data);
        return;
    }
    if(data[0] == "sellcorrection"){
        sellCorrection(data);
        return;
    }
    else if(data[0] == "wincorrection"){
        winCorrection(data);
        return;
    }
}

//Слот синхронизирует дату с ПК(только при закрытой смене)
void Printer::dateSyncSlot(int day, int month, int year)
{
    printReportZ();
    _ifptr->put_Date(day, month, year);
    if(_ifptr->SetDate() < 0)
    {
        int rc = EC_OK;
        _ifptr->get_ResultCode(&rc);
        if(rc == EC_3893)
            _ifptr->SetDate();
        else
            emit logSend(checkError());
    }
}

//Слот синхронизирует время с ПК
void Printer::timeSyncSlot(int hour, int minute, int second)
{
    _ifptr->put_UserPassword(L"29");
    _ifptr->put_Mode(TED::Fptr::ModeRegistration);
    _ifptr->SetMode();
    _ifptr->CancelCheck();

    _ifptr->put_Time(hour, minute, second);
    if(_ifptr->SetTime() < 0)
        emit logSend(checkError());
}

//Метод формирует штрихкод
int Printer::printBarcode(TED::Fptr::BarcodeType type, const wchar_t *barcode, double scale)
{
    _ifptr->put_Alignment(TED::Fptr::AlignmentCenter);
    _ifptr->put_BarcodeType(type);
    _ifptr->put_Barcode(barcode);
    _ifptr->put_Height(0);
    _ifptr->put_BarcodeVersion(0);
    _ifptr->put_BarcodePrintType(TED::Fptr::BarcodeAuto);
    _ifptr->put_PrintBarcodeText(0);
    _ifptr->put_BarcodeControlCode(1);
    _ifptr->put_Scale(scale);
    _ifptr->put_BarcodeCorrection(0);
    _ifptr->put_BarcodeColumns(3);
    _ifptr->put_BarcodeRows(1);
    _ifptr->put_BarcodeProportions(50);
    _ifptr->put_BarcodeUseProportions(1);
    _ifptr->put_BarcodePackingMode(TED::Fptr::BarcodePDF417PackingModeDefault);
    _ifptr->put_BarcodePixelProportions(150);
    if(_ifptr->PrintBarcode() < 0)
    {
        socketData = "{\"error\": \""+checkError()+"\"}";
        return -1;
    }
    return 0;
}

//Метод открывает чек
int Printer::openCheck(TED::Fptr::ChequeType type)
{
    _ifptr->put_UserPassword(L"29");
    _ifptr->put_Mode(TED::Fptr::ModeRegistration);
    _ifptr->SetMode();
    _ifptr->put_CheckType(type);
    if(_ifptr->OpenCheck() < 0)
    {
        socketData = "{\"error\": \""+checkError()+"\"}";
        return -1;
    }
    return 0;
}

//Метод закрывает чек
int Printer::closeCheck(int typeClose)
{
    _ifptr->put_TypeClose(typeClose);
    if(_ifptr->CloseCheck() < 0)
    {
        socketData = "{\"error\": \""+checkError()+"\"}";
        return -1;
    }
    return 0;
}

//Метод регистрирует чек прихода
int Printer::registrationFZ54(const wchar_t *name, double price, double quantity, double positionSum, int taxNumber, int department)
{
    _ifptr->put_PositionSum(positionSum);
    _ifptr->put_Quantity(quantity);
    _ifptr->put_Price(price);
    _ifptr->put_TaxNumber(taxNumber);
    _ifptr->put_TextWrap(TED::Fptr::TextWrapWord);
    _ifptr->put_Name(name);
    _ifptr->put_Department(department);
    if(_ifptr->Registration() < 0)
    {
        socketData = "{\"error\": \""+checkError()+"\"}";
        return -1;
    }
    return 0;
}

//Метод регистрирует оплату
int Printer::payment(double sum, int type)
{
    _ifptr->put_Summ(sum);
    _ifptr->put_TypeClose(type);
    if(_ifptr->Payment() < 0)
    {
        socketData = "{\"error\": \""+checkError()+"\"}";
        return -1;
    }
    return 0;
}

//Метод обрабатывает ошибки
QString Printer::checkError()
{
    int rc = EC_OK;
    _ifptr->get_ResultCode(&rc);
    if(rc < 0)
    {
        QString resultDescription, badParamDescription;
        QVector<wchar_t> v(256);
        int size = _ifptr->get_ResultDescription(&v[0], v.size());
        if(size > v.size())
        {
            v.clear();
            v.resize(size + 1);
            _ifptr->get_ResultDescription(&v[0], v.size());
        }
        resultDescription = QString::fromWCharArray(&v[0]);
        if(rc == EC_INVALID_PARAM)
        {
            QVector<wchar_t> v(256);
            int size = _ifptr->get_BadParamDescription(&v[0], v.size());
            if (size > v.size())
            {
                v.clear();
                v.resize(size + 1);
                _ifptr->get_ResultDescription(&v[0], v.size());
            }
            badParamDescription = QString::fromWCharArray(&v[0]);
        }
        if(badParamDescription.isEmpty())
            return "["+QString::number(rc)+"] "+resultDescription;
        else
            return "["+QString::number(rc)+"] "+resultDescription+" ("+badParamDescription+")";
    }
    return "";
}

