#include "logmessage.h"
#include "ui_logmessage.h"

LogMessage::LogMessage(QWidget *parent) :
    QDialog(parent),
    _ui(new Ui::LogMessage)
{
    _ui->setupUi(this);
    this->setModal(false);
}

LogMessage::~LogMessage()
{
    delete _ui;
}

void LogMessage::message(const QString &logMessage)
{
    this->setWindowTitle("Error");
    _ui->label->setText(logMessage);
}
