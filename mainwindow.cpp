#include "mainwindow.h"
#include "ui_mainwindow.h"


#include <QPixmap>
#include <QMessageBox>
#include <QSettings>
#include <QCloseEvent>


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


//    ui->lbDBConnected->setStyleSheet("color: green;");
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
    settings.setValue("leConnURL", ui->leConnURL->text());
    settings.setValue("leUser", ui->leUser->text());
    settings.setValue("lePassword", ui->lePassword->text());
    settings.setValue("leDatabase", ui->leDatabase->text());
}

void MainWindow::loadSettings()
{
    QSettings settings("Nekosilverfox", "BlockStoreTester", this);

    ui->leHost->setText(settings.value("leHost", "localhost").toString());
    ui->lePort->setText(settings.value("lePort", "5432").toString());
    ui->leConnURL->setText(settings.value("leConnURL", "postgresql://localhost").toString());
    ui->leUser->setText(settings.value("leUser", "").toString());
    ui->lePassword->setText(settings.value("lePassword", "").toString());
    ui->leDatabase->setText(settings.value("leDatabase", "dbBlockStoreTester").toString());


}

void MainWindow::closeEvent(QCloseEvent* event)
{
    event->accept();
    QMainWindow::closeEvent(event);

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

