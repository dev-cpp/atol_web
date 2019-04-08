#ifndef SETTINGS_H
#define SETTINGS_H

#include <QFile>
#include <QDialog>
#include <QStringList>
#include <QTextStream>
#include <QApplication>

namespace Ui {
class Settings;
}

class Settings : public QDialog
{
    Q_OBJECT

public:
    explicit Settings(QWidget *parent = nullptr);
    ~Settings();

signals:
    void reload();

private slots:
    void fillSettingsFile();
    void reloadPrinter();

private:
    Ui::Settings* _ui;

    void createSettingsFile();
    void readSettingsFile();
};

#endif // SETTINGS_H
