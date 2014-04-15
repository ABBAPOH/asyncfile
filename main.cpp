#include <QApplication>

#include <QFile>
#include <QDebug>
#include <QThread>

#include <Windows.h>
#include <tchar.h>
#include <stdio.h>

#include "mainwindow.h"

const char *const filePath = "C:/Users/arch/Programming/asyncfile/freedesktop.org.xml";
//const char *const filePath = "C:/Users/arch/Programming/asyncfile/asyncfile.h";

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    MainWindow w;
    w.show();

    qDebug() << "main" << QThread::currentThreadId();

    AsyncFile file(filePath);
    if (!file.open(QIODevice::ReadOnly))
        return 1;

    qDebug() << "at end" << file.atEnd();
    QByteArray data;
    while (file.pos() < file.size()) {
        file.waitForReadyRead(1000);
        data.append(file.read(file.bytesAvailable()));
//        qDebug() << "read" << file.read(file.bytesAvailable());
    }
    qDebug() << QString::fromUtf8(data).right(1*1024);
    qDebug() << file.pos() << file.size();

    return a.exec();
}
