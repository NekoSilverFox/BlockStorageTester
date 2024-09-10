#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QPixmap>
#include <QMessageBox>
#include <QSettings>
#include <QCloseEvent>
#include <QSqlQuery>
#include <QSqlError>
#include <QFileDialog>


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    /* 初始化 */
    writeInfoLog(QString("Available drivers: %1").arg(QSqlDatabase::drivers().join(" ")));
    writeInfoLog("Start init");

    // setWindowIcon(QIcon(":/icons/logo.png"));
    ui->lbPicSQLServer->setPixmap(QPixmap(":/icons/sql-server.png"));
    ui->lbPicSQLServer->setScaledContents(true);

    ui->lbDBConnected->setStyleSheet("color: red;");
    ui->lbDBUsing->setStyleSheet("color: red;");

    _curDBName = "";
    _isFinalConnDB = false;

    /* 点击连接按钮 */
    connect(ui->btnConnectDB, &QPushButton::clicked, this, &MainWindow::autoConnectionDBModule);
    connect(ui->btnDisconnect, &QPushButton::clicked, this, &MainWindow::disconnectDatabase);

    connect(ui->btnSelectSourceFile, &QPushButton::clicked, this, &MainWindow::selectSourceFile);

    /* TODO 点击运行禁用按钮 */
    connect(ui->btnRunTest, &QPushButton::clicked, this, [=](){setActivityWidget(false);});

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
    settings.setValue("cbAutoDropDB", ui->cbAutoDropDB->isChecked());

    settings.setValue("leSourceFile", ui->leSourceFile->text());
    settings.setValue("leBlockFile", ui->leBlockFile->text());

    settings.setValue("cbBlockSize", ui->cbBlockSize->currentIndex());
    settings.setValue("cbHashAlg", ui->cbHashAlg->currentIndex());
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
    ui->cbAutoDropDB->setChecked(settings.value("cbAutoDropDB", true).toBool());

    ui->leSourceFile->setText(settings.value("leSourceFile", "").toString());
    ui->leBlockFile->setText(settings.value("leBlockFile", "").toString());

    ui->cbBlockSize->setCurrentIndex(settings.value("cbBlockSize", 0).toInt());
    ui->cbHashAlg->setCurrentIndex(settings.value("cbHashAlg", 0).toInt());

    writeInfoLog("Success load settings");
}

// https://ru.stackoverflow.com/questions/1478871/qpsql-driver-not-found
bool MainWindow::connectDatabase(const QString& host, const int port, const QString& driver, const QString& user, const QString& pwd, const QString& database)
{
    _curDB = QSqlDatabase::addDatabase(driver, QSqlDatabase::defaultConnection);
    _curDB.setHostName(host);
    _curDB.setPort(port);
    _curDB.setUserName(user);
    _curDB.setPassword(pwd);

    if (!database.isEmpty())
    {
        _curDB.setDatabaseName(database);
    }

    if (_curDB.open())
    {
        ui->lbDBConnected->setStyleSheet("color: green;");
        writeInfoLog(QString("Successed connect to database %1 from %2:%3").arg(database, host, QString::number(port)));
        return true;
    }
    else
    {
        ui->lbDBConnected->setStyleSheet("color: red;");
        QMessageBox::warning(this, "DB Connect failed",
                             QString("Cannot connect to datebase %1 from %2:%3").arg(database, host, QString::number(port)));
        writeErrorLog(QString("Cannot connect to datebase %1 from %2:%3").arg(database, host, QString::number(port)));
        return false;
    }
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

/** 设置小部件的可操作性
 * @brief MainWindow::setActivityWidget
 * @param activity
 */
void MainWindow::setActivityWidget(const bool activity)
{
    ui->leUser->setReadOnly(!activity);
    ui->lePassword->setReadOnly(!activity);
    ui->leHost->setReadOnly(!activity);
    ui->lePort->setReadOnly(!activity);
    ui->leDatabase->setReadOnly(!activity);

    ui->btnConnectDB->setEnabled(activity);
    ui->btnDisconnect->setEnabled(activity);

    ui->leSourceFile->setReadOnly(!activity);
    ui->leBlockFile->setReadOnly(!activity);
    ui->btnSelectSourceFile->setEnabled(activity);
    ui->btnSelectBlockFile->setEnabled(activity);

    ui->cbBlockSize->setEnabled(activity);
    ui->cbHashAlg->setEnabled(activity);
    ui->btnRunTest->setEnabled(activity);
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
        q.clear();
        return true;
    }

    writeErrorLog(QString("Database `%1` do not exist").arg(db));
    q.clear();
    return false;
}

bool MainWindow::createDatabase(const QString& db)
{
    writeInfoLog(QString("Start to create database %1").arg(db));
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


bool MainWindow::disconnectDatabase()
{
    // 检查数据库连接是否有效并已打开
    if (_curDB.isValid() && _curDB.open())
    {
        _curDB.close();
        _curDB = QSqlDatabase(); // 将 _curDB 重置为空，以解除与实际数据库连接的关联
        QSqlDatabase::removeDatabase(QSqlDatabase::defaultConnection);
        _isFinalConnDB = false;

        ui->lbDBUsing->setStyleSheet("color: red;");
        ui->lbDBConnected->setStyleSheet("color: red;");
        writeInfoLog(QString("Disconnect database %1").arg(_curDB.databaseName()));
        return true;
    }

    writeWarningLog(QString("Failed disconnect database %1").arg(_curDB.databaseName()));
    return false;
}

void MainWindow::closeEvent(QCloseEvent* event)
{
    /* 自动删除使用的数据库 */
    if (ui->cbAutoDropDB->isChecked() && _isFinalConnDB && !_curDBName.isEmpty())
    {
        QString dropDBName = _curDB.databaseName();

        disconnectDatabase();
        connectDatabase(_host, _port,_driver, _user, _password, "");

        QSqlQuery q;
        QString sql = QString("DROP DATABASE \"%1\";").arg(dropDBName);
        bool succ = q.exec(sql);
        if (succ)
        {
            qDebug() << QString("Successed drop database `%1`").arg(dropDBName);
        }
        else
        {
            qDebug() << QString("Failed drop database `%1`").arg(dropDBName);
        }
    }

    disconnectDatabase();

    event->accept();
    QMainWindow::closeEvent(event);

    saveSettings();
}


void MainWindow::on_actionAbout_triggered()
{
    QMessageBox::about(this, "About Block Storage Tester",
                       "Project Block Storage Tester<br><br>"
                       "Author: Meng Jianing<br>"
                       "Source Code:<br>"
                       "<a href=\"https://github.com/NekoSilverFox/BlockStorageTester\">[Github] BlockStorageTester</a>"
                       "<br><br>"
                       "Made on Qt Creator 14.0.1<br>"
                       "Based on Qt 6.7.2"
                       "<br><br>"
                       "License: Apache License 2.0");
}


void MainWindow::autoConnectionDBModule()
{
    /** 先尝试连接数据库（使用默认数据库名）
     */
    if (ui->leHost->text().isEmpty()
        || ui->lePort->text().isEmpty()
        || ui->leDriver->text().isEmpty()
        || ui->leUser->text().isEmpty()
        || ui->lePassword->text().isEmpty()
        || ui->leDatabase->text().isEmpty())
    {
        writeErrorLog("Fail connect to database");
        QMessageBox::critical(this, "Error", "Please input all info about the database!");
        return;
    }
    _host = ui->leHost->text();
    _port = ui->lePort->text().toInt();
    _driver = ui->leDriver->text();
    _user = ui->leUser->text();
    _password = ui->lePassword->text();
    _curDBName = ui->leDatabase->text().toLower();  // PostgreSQL数据库只能小写

    disconnectDatabase();
    connectDatabase(_host, _port,_driver, _user, _password, "");

    /**
     * 检查用户指定的数据库是否存在
     */
    if (!isDatabaseExist(_curDBName))
    {
        int ret = QMessageBox::question(this, "Automatic create database",
                                        QString("Database `%1` do not exist, do you want automatic create now?").arg(_curDBName),
                                        QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes);

        if (QMessageBox::Yes == ret)
        {
            bool succ = createDatabase(_curDBName);
            if (!succ)
            {
                return;
            }
        }
        else
        {
            writeWarningLog(QString("Automatic create database `%1` cancel").arg(_curDBName));
            return;
        }
    }

    /**
     * 重新连接为用户指定的数据库
     */
    disconnectDatabase();
    writeInfoLog(QString("Re-connect database `%1` ...").arg(_curDBName));
    ui->lbDBConnected->setStyleSheet("color: orange;");

    _isFinalConnDB = connectDatabase(_host, _port,_driver, _user, _password, _curDBName);
    if (_isFinalConnDB)
    {
        ui->lbDBUsing->setStyleSheet("color: green;");
        QMessageBox::information(this, "Success connected",
                                 QString("Successed connected to datebase %1 from %2:%3").arg(_curDBName, _host, QString::number(_port)));
    }

}


void MainWindow::selectSourceFile()
{
    QString path = QFileDialog::getOpenFileName(this, "Select file", QDir::homePath());
    if (path.isEmpty())
    {
        QMessageBox::warning(this, "Warning", "Do not selected any file!");
        return;
    }
}

/** 执行测试
 * @brief MainWindow::runTestModule
 */
void MainWindow::runTestModule()
{

}


