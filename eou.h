#ifndef EOU_H
#define EOU_H

#include <QObject>
#include <QProcess>
#include <QApplication>

class EoU : public QObject
{
    Q_OBJECT
public:
    explicit EoU(QObject *parent = nullptr);
    ~EoU();

signals:

public slots:
    void createEoU();

private:
    QProcess* _process;
};

#endif // EOU_H
