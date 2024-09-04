#include "mainwindow.h"
#include "ui_mainwindow.h"


#include <QPixmap>
#include <QMessageBox>
#include <QSettings>
#include <QCloseEvent>
#include <QMessageBox>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    /* 初始化 */
    ui->lbPicSQLServer->setPixmap(QPixmap(":/icons/sql-server.png"));
    ui->lbPicSQLServer->setScaledContents(true);


    ui->lbDBConnected->setStyleSheet("color: red;");
    ui->lbDBExisted->setStyleSheet("color: red;");

    connect(ui->btnConnectDB, &QPushButton::clicked, this, [=](){
        if (ui->leHost->text().isEmpty()
            || ui->lePort->text().isEmpty()
            || ui->leDriver->text().isEmpty()
            || ui->leUser->text().isEmpty()
            || ui->lePassword->text().isEmpty())
        {
            QMessageBox::critical(this, "Error", "Please input all info about the database!");
        }
        else
        {
            if (db.open())
            {
                db.close();
                QSqlDatabase::removeDatabase(QSqlDatabase::defaultConnection);

                qDebug() << "db close";
            }

            bool isSucc = connectDatabase(ui->leHost->text(), ui->lePort->text().toInt(),
                                          ui->leDriver->text(), ui->leUser->text(), ui->lePassword->text());

            if (isSucc)
            {
                ui->lbDBConnected->setStyleSheet("color: green;");
            }
            else
            {
                ui->lbDBConnected->setStyleSheet("color: red;");
                QMessageBox::warning(this, "DB Connect fail", QString("Cannot connect to datebase %1:%2").arg(ui->leHost->text(), ui->lePort->text()));
                // https://ru.stackoverflow.com/questions/1478871/qpsql-driver-not-found
            }
        }
    });


//    ui->lbDBExisted->setStyleSheet("color: green;");

    loadSettings();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::saveSettings()
{
    QSettings settings("Nekosilverfox", "BlockStoreTester", this);

    settings.setValue("leHost", ui->leHost->text());
    settings.setValue("lePort", ui->lePort->text());
    settings.setValue("leDriver", ui->leDriver->text());
    settings.setValue("leUser", ui->leUser->text());
    settings.setValue("lePassword", ui->lePassword->text());
    settings.setValue("leDatabase", ui->leDatabase->text());
}

void MainWindow::loadSettings()
{
    QSettings settings("Nekosilverfox", "BlockStoreTester", this);

    ui->leHost->setText(settings.value("leHost", "localhost").toString());
    ui->lePort->setText(settings.value("lePort", "5432").toString());
    ui->leDriver->setText(settings.value("leDriver", "QPSQL").toString());
    ui->leUser->setText(settings.value("leUser", "").toString());
    ui->lePassword->setText(settings.value("lePassword", "").toString());
    ui->leDatabase->setText(settings.value("leDatabase", "dbBlockStoreTester").toString());
}

bool MainWindow::connectDatabase(QString host, int port, QString driver, QString user, QString pwd)
{
    db = QSqlDatabase::addDatabase(driver);
    db.setHostName(host);
    db.setPort(port);
    db.setUserName(user);
    db.setPassword(pwd);

    return db.open();
}

void MainWindow::closeEvent(QCloseEvent* event)
{
    event->accept();
    QMainWindow::closeEvent(event);

    if (db.open())
    {
        db.close();
        QSqlDatabase::removeDatabase(QSqlDatabase::defaultConnection);
    }

    saveSettings();
}


void MainWindow::on_actionAbout_triggered()
{
    QMessageBox::about(this, "About Block Storage Tester",
                       "Project Block Storage Tester<br><br>"
                       "Source Code:<br>"
                       "<a href=\"https://github.com/NekoSilverFox/BlockStorageTester\">[Github] BlockStorageTester</a>"
                       "<br><br>"
                       "License: Apache License 2.0"
                       "<br><br>"
                       "Made on Qt 6.4.3");
}

