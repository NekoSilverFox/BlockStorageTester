#include "mainwindow.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    qDebug() << "Available drivers:" << QSqlDatabase::drivers();

    MainWindow w;
    w.setWindowTitle("Block Storage Tester by MengJianing 5140904/30202 SPbSTU");
    w.show();
    return a.exec();
}
