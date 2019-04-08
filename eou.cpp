#include "eou.h"
#include <windows.h>

EoU::EoU(QObject *parent) : QObject(parent)
{

}

EoU::~EoU()
{
    delete _process;
}

void EoU::createEoU()
{
    _process = new QProcess(this);
    _process->start(QApplication::applicationDirPath() + "/EoU/EthOverUsb.exe -e");
}
