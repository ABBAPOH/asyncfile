#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QDebug>
#include <QMessageBox>

const char *const filePath = "C:/Users/arch/Programming/asyncfile/freedesktop.org.xml";

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    m_file(AsyncFile(filePath))
{
    ui->setupUi(this);

    if (!m_file.open(QIODevice::ReadOnly))
        QMessageBox::warning(this, tr("Warning"), tr("Can't open file: %1").arg(m_file.errorString()), QMessageBox::Close);

    connect(&m_file, &QIODevice::readyRead, this, &MainWindow::onReadyRead);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::onReadyRead()
{
    QByteArray data = m_file.read(m_file.bytesAvailable());
    qDebug() << "onReadyRead" << data;

    ui->plainTextEdit->appendPlainText(QString::fromUtf8(data));
//    qApp->processEvents();
}
