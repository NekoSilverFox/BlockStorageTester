#include "mainwindow.h"
#include "ui_mainwindow.h"


#include <QPixmap>
#include <QMessageBox>
#include <QSettings>
#include <QCloseEvent>
#include <QMessageBox>
#include <QSqlQuery>
#include <QSqlError>


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    /* 初始化 */
    writeInfoLog(QString("Available drivers: %1").arg(QSqlDatabase::drivers().join(" ")));
    writeInfoLog("Start init");

    ui->lbPicSQLServer->setPixmap(QPixmap(":/icons/sql-server.png"));
    ui->lbPicSQLServer->setScaledContents(true);


    ui->lbDBConnected->setStyleSheet("color: red;");
    ui->lbDBUsing->setStyleSheet("color: red;");

    /* 点击连接按钮 */
    connect(ui->btnConnectDB, &QPushButton::clicked, this, [=](){
        if (ui->leHost->text().isEmpty()
            || ui->lePort->text().isEmpty()
            || ui->leDriver->text().isEmpty()
            || ui->leUser->text().isEmpty()
            || ui->lePassword->text().isEmpty())
        {
            writeErrorLog("Fail connect to database");
            QMessageBox::critical(this, "Error", "Please input all info about the database!");
            return;
        }

        if (db.open())
        {
            db.close();
            QSqlDatabase::removeDatabase(QSqlDatabase::defaultConnection);
            writeWarningLog("Disconnect database");
        }

        bool isSucc = connectDatabase(ui->leHost->text(), ui->lePort->text().toInt(),
                                      ui->leDriver->text(), ui->leUser->text(), ui->lePassword->text());

        if (isSucc)
        {
            ui->lbDBConnected->setStyleSheet("color: green;");
            writeInfoLog("Success connect to database");
        }
        else
        {
            ui->lbDBConnected->setStyleSheet("color: red;");
            QMessageBox::warning(this, "DB Connect fail", QString("Cannot connect to datebase %1:%2").arg(ui->leHost->text(), ui->lePort->text()));
            writeErrorLog(QString("Cannot connect to datebase %1:%2").arg(ui->leHost->text(), ui->lePort->text()));
            // https://ru.stackoverflow.com/questions/1478871/qpsql-driver-not-found
        }
    });

    /* 点击使用数据库 */
    connect(ui->btnUseDB, &QPushButton::clicked, this, [&](){
        if (!db.open())
        {
            writeErrorLog("Did not connect to database");
            QMessageBox::critical(this, "Error", "Did not connect to database, please connect database at first!");
            return;
        }

        if (ui->leDatabase->text().isEmpty())
        {
            writeErrorLog("Database name is empty");
            QMessageBox::critical(this, "Error", "The database name is empty!");
            return;
        }

        curUsingDB = ui->leDatabase->text().toLower();  // PostgreSQL数据库只能小写
        if (!isDatabaseExist(curUsingDB))
        {
            int ret = QMessageBox::question(this, "Automatic create database",
                                            QString("Database `%1` do not exist, do you want automatic create now?").arg(curUsingDB),
                                            QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes);

            if (QMessageBox::Yes == ret)
            {
                bool succ = createDatabase(curUsingDB);
                if (succ)
                {
                    useDatabase(curUsingDB);
                    return;
                }
                else
                {
                    return;
                }
            }
            else
            {
                writeWarningLog(QString("Automatic create database `%1` cancel").arg(curUsingDB));
                return;
            }
        }
    });


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

    writeInfoLog("Success load settings");
}

bool MainWindow::connectDatabase(const QString& host, const int port, const QString& driver, const QString& user, const QString& pwd)
{
    db = QSqlDatabase::addDatabase(driver);
    db.setHostName(host);
    db.setPort(port);
    db.setUserName(user);
    db.setPassword(pwd);

    return db.open();
}

void MainWindow::writeInfoLog(const QString& msg)
{
    ui->txbLog->setTextColor(Qt::black);
    ui->txbLog->append(QString("[INFO] %1").arg(msg));
}

void MainWindow::writeWarningLog(const QString& msg)
{
    ui->txbLog->setTextColor(Qt::darkYellow);
    ui->txbLog->append(QString("[WARNING] %1").arg(msg));
}

void MainWindow::writeErrorLog(const QString& msg)
{
    ui->txbLog->setTextColor(Qt::red);
    ui->txbLog->append(QString("[ERROR] %1").arg(msg));
}

bool MainWindow::isDatabaseExist(const QString& db)
{
    QString sql = QString("SELECT 1 FROM pg_database WHERE datname = '%1';").arg(db);
    QSqlQuery q(sql);
    writeInfoLog(QString("Run SQL: %1").arg(sql));

    q.next();
    if (q.value(0) == 1)
    {
        writeInfoLog(QString("Database `%1` exist").arg(db));
        return true;
    }

    writeErrorLog(QString("Database `%1` do not exist").arg(db));
    return false;
}

bool MainWindow::createDatabase(const QString& db)
{
    QString sql = QString("CREATE DATABASE \"%1\";").arg(db);
    QSqlQuery q;
    writeInfoLog(QString("Run SQL: %1").arg(sql));

    bool succ = q.exec(sql);
    if (succ)
    {
        writeInfoLog(QString("Successed create database `%1`").arg(db));
        return true;
    }
    else
    {
        writeErrorLog(QString("Failed to create database `%1`: %2").arg(db, q.lastError().text()));
        return false;
    }
}

bool MainWindow::useDatabase(const QString& db)
{
    return true;
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

