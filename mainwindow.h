#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

#include "asyncfile.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private slots:
    void onReadyRead();

private:
    Ui::MainWindow *ui;
    AsyncFile m_file;
};

#endif // MAINWINDOW_H
