#include <QTranslator>
#include <QApplication>

#include "mainwindow.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    QTranslator *qt_translator = new QTranslator;
    if(qt_translator->load(":trs/qt_ru.qm"))
    {
        a.installTranslator(qt_translator);
    }
    MainWindow w;

    return a.exec();
}
