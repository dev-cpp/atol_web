#include "settings.h"
#include "ui_settings.h"

Settings::Settings(QWidget *parent) :
    QDialog(parent),
    _ui(new Ui::Settings)
{
    _ui->setupUi(this);

    QRegExp rx("^\\d\\d?$");
    QValidator *validator = new QRegExpValidator(rx, this);
    _ui->com1->setValidator(validator);
    _ui->com2->setValidator(validator);

    createSettingsFile();
    readSettingsFile();

    connect(_ui->okButton,SIGNAL(clicked(bool)),this,SLOT(fillSettingsFile()));
    connect(_ui->okButton,SIGNAL(clicked(bool)),this,SLOT(reloadPrinter()));
    connect(_ui->okButton,SIGNAL(clicked(bool)),this,SLOT(reject()));
}

Settings::~Settings()
{
    delete _ui;
}

//Слот заполняет файл настроек
void Settings::fillSettingsFile()
{
    QStringList buffer;
    QFile settingsFile(QApplication::applicationDirPath()+"/settings.ini");
    if(!settingsFile.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        return;
    }
    if(_ui->atol22->isChecked())
    {
        buffer.append("1");
    }
    if(_ui->atol30->isChecked())
    {
        buffer.append("2");
    }
    QTextStream stream(&settingsFile);
    if(!_ui->com1->text().isEmpty())
        buffer.append(_ui->com1->text());
    if(!_ui->com2->text().isEmpty())
        buffer.append(_ui->com2->text());
    if(!buffer.isEmpty() && buffer.length() == 3)
    {
        stream << buffer[0] << "\n";
        stream << buffer[1] << "\n";
        stream << buffer[2];
    }
    settingsFile.flush();
    settingsFile.close();
}

//Слот посылает сигнал пересоздания обьекта принтера
void Settings::reloadPrinter()
{
    emit reload();
}

//Метод создает файл настроек
void Settings::createSettingsFile()
{
    QFile settingsFile(QApplication::applicationDirPath()+"/settings.ini");
    if(!settingsFile.exists())
    {
        settingsFile.open(QIODevice::WriteOnly);
        settingsFile.close();
        return;
    }
}

//Метод читает файл настроек
void Settings::readSettingsFile()
{
    QStringList buffer;
    QFile settingsFile(QApplication::applicationDirPath()+"/settings.ini");
    if(!settingsFile.open(QIODevice::ReadOnly))
    {
        return;
    }
    while(!settingsFile.atEnd())
    {
        buffer.append(settingsFile.readLine().replace("\r\n", ""));
    }
    settingsFile.close();
    if(!buffer.isEmpty() && buffer.length() == 3)
    {
        if(buffer[0] == "1")
        {
            _ui->atol22->setChecked(true);
        }
        if(buffer[0] == "2")
        {
            _ui->atol30->setChecked(true);
        }
        _ui->com1->setText(buffer[1]);
        _ui->com2->setText(buffer[2]);
    }
    return;
}
