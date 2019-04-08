#ifndef LOGMESSAGE_H
#define LOGMESSAGE_H

#include <QDialog>
#include <QString>

namespace Ui {
class LogMessage;
}

class LogMessage : public QDialog
{
    Q_OBJECT

public:
    explicit LogMessage(QWidget *parent = nullptr);
    ~LogMessage();

public slots:
    void message(const QString &logMessage);

private:
    Ui::LogMessage* _ui;
};

#endif // LOGMESSAGE_H
