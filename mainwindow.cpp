#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QPixmap>
#include <QMessageBox>
#include <QSettings>
#include <QCloseEvent>
#include <QSqlQuery>
#include <QSqlError>
#include <QFileDialog>

#include "HashAlgorithm.h"


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    /* 初始化 */
    writeInfoLog(QString("Available drivers: %1").arg(QSqlDatabase::drivers().join(" ")));
    writeInfoLog("Start init");

    setWindowIcon(QIcon(":/icons/logo.png"));
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
    connect(ui->btnSelectBlockFile, &QPushButton::clicked, this, &MainWindow::selectBlockFile);

    /* TODO 点击运行禁用按钮 */
    connect(ui->btnRunTest, &QPushButton::clicked, this, &MainWindow::testBlockWritePerformanceModule);

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

    writeInfoLog("Successed save settings");
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

    writeInfoLog("Successed load settings");
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

bool MainWindow::createTable(const QString& tbName)
{
    writeInfoLog(QString("Create tabel `%1`").arg(tbName));

    QSqlQuery q;
    QString sql = QString("CREATE TABLE %1 ("
                          "hash_value BYTEA NOT NULL,"
                          "file_name TEXT NOT NULL,"
                          "location INTEGER NOT NULL,"
                          "counter NUMERIC(1000, 0) NOT NULL DEFAULT 1);").arg(tbName);

    writeInfoLog(QString("Create table `%1`").arg(tbName));
    writeInfoLog(QString("\t-> Run SQL: %1").arg(sql));

    if (q.exec(sql))
    {
        writeInfoLog(QString("\t-> Successed table `%1`").arg(tbName));
        return true;
    }

    writeErrorLog(QString("\t-> Failed to create table `%1`: %2").arg(tbName, q.lastError().text()));
    return false;
}

bool MainWindow::insertNewRow(const QString& tbName, const QByteArray& hashValue, const QString& fileName, const int location)
{
    writeInfoLog(QString("Insert new row to tabel `%1`").arg(tbName));
    // 创建查询对象
    QSqlQuery q(QSqlDatabase::database(QSqlDatabase::defaultConnection));

    // 准备插入语句，使用参数绑定防止 SQL 注入
    q.prepare(QString("INSERT INTO %1 (hash_value, file_name, location, counter) "
                      "VALUES (:hash_value, :file_name, :location, :counter)").arg(tbName));

    // 绑定参数
    q.bindValue(":hash_value", hashValue);  // 直接绑定 QByteArray
    q.bindValue(":file_name", fileName);
    q.bindValue(":location", location);
    q.bindValue(":counter", 1);

    // 执行插入
    if (q.exec())
    {
        writeInfoLog("\t-> Successed insert new row");
        return true;
    }

    writeErrorLog(QString("\t-> Failed to insert new row: %1").arg(q.lastError().text()));
    return false;
}

int MainWindow::getHashRepeatTimes(const QString& tbName, const QByteArray &hashValue)
{
    writeInfoLog("Get hash repeat times");

    // 创建查询对象
    QSqlQuery q(QSqlDatabase::database(QSqlDatabase::defaultConnection));

    // 准备查询语句，查找相同的哈希值
    q.prepare(QString("SELECT counter FROM %1 WHERE hash_value = :hash_value").arg(tbName));

    // 绑定哈希值参数
    q.bindValue(":hash_value", hashValue);

    // 执行查询
    if (!q.exec()) {
        writeErrorLog((QString("\t-> Failed to get HashRepeatTimes: %1").arg(q.lastError().text())));
        return -1;  // 返回 -1 表示查询失败
    }

    // 检查是否存在结果
    if (q.next())
    {
        int counter = q.value(0).toInt();  // 获取重复次数
        writeInfoLog(QString("\t-> Find the same hash, repeat times: %1").arg(counter));
        return counter;
    }
    else
    {
        writeInfoLog("\t-> Find the same hash, repeat times: %1");
        return 0;  // 返回 0 表示没有找到重复的记录
    }
}

bool MainWindow::updateCounter(const QString& tbName, const QByteArray &hashValue, int count)
{
    writeInfoLog("Update counter");
    // 创建查询对象
    QSqlQuery q(QSqlDatabase::database(QSqlDatabase::defaultConnection));

    // 准备更新语句，更新指定哈希值对应的 counter
    q.prepare(QString("UPDATE %1 SET counter = :counter WHERE hash_value = :hash_value").arg(tbName));

    // 绑定参数
    q.bindValue(":counter", count);
    q.bindValue(":hash_value", hashValue);

    // 执行更新
    if (!q.exec()) {
        writeErrorLog(QString("\t-> Update failure: %1").arg(q.lastError().text()));
        return false;
    }

    // 检查是否有行受影响
    if (q.numRowsAffected() > 0)
    {
        writeInfoLog(QString("\t-> Update successful! Number of rows affected: %1").arg(q.numRowsAffected()));
        return true;
    }
    else
    {
        writeErrorLog("\t-> No matching hash found, update not performed");
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
    bool isSucc = connectDatabase(_host, _port,_driver, _user, _password, "");
    if (!isSucc)
    {
        return;
    }

    /**
     * 检查用户指定的数据库是否存在
     */
    if (!isDatabaseExist(_curDBName))
    {
        /* 数据库成功连接 && 数据库不存在 */
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
    writeInfoLog(QString("Re-connect to database `%1` ...").arg(_curDBName));
    ui->lbDBConnected->setStyleSheet("color: orange;");

    _isFinalConnDB = connectDatabase(_host, _port,_driver, _user, _password, _curDBName);
    if (_isFinalConnDB)
    {
        ui->lbDBUsing->setStyleSheet("color: green;");
        QMessageBox::information(this, "Success connected",
                                 QString("Successed connected to datebase %1 from %2:%3").arg(_curDBName, _host, QString::number(_port)));
    }
}

void MainWindow::testBlockWritePerformanceModule()
{
    writeInfoLog("Start test block write performance");

    /* 数据库无连接 */
    if (!_isFinalConnDB)
    {
        writeErrorLog("↳ Database do not connected, test end");
        QMessageBox::warning(this, "Warning", "Database do not connected!");
        return;
    }

    /* 没有选择文件或保存路径 */
    if (ui->leSourceFile->text().isEmpty() || ui->leBlockFile->text().isEmpty())
    {
        writeErrorLog("↳ Source file or block file path is empty");
        QMessageBox::warning(this, "Warning", "Source file or block file path is empty!");
        return;
    }

    /* 为了避免意外操作，给暂时禁用按钮 */
    setActivityWidget(false);

    /* 打开源文件 */
    QFile sourceFile(ui->leSourceFile->text());
    QFileInfo sourceInfo;
    qint64 source_size;  // Bytes
    QDataStream in;
    if (sourceFile.open(QIODevice::ReadOnly))
    {
        in.setDevice(&sourceFile);
        in.setVersion(QDataStream::Qt_DefaultCompiledVersion);  // 设置流的版本（可以根据实际情况设置，通常用于处理跨版本兼容性）
        sourceInfo.setFile(sourceFile);
        source_size = sourceInfo.size();

        writeInfoLog(QString("↳ Successed open file %1, size %2 Bytes").arg(sourceInfo.filePath(), QString::number(source_size)));
    }
    else
    {
        writeErrorLog(QString("↳ Can not open file %1").arg(sourceInfo.filePath()));
        QMessageBox::critical(this, "Error", QString("Can not open file %1").arg(sourceInfo.filePath()));
        return;
    }

    /* 创建块文件 */
    QFile blockFile(ui->leBlockFile->text());
    QFileInfo blockInfo;
    QDataStream out;
    if (blockFile.open(QIODevice::WriteOnly))
    {
        out.setDevice(&blockFile);
        out.setVersion(QDataStream::Qt_DefaultCompiledVersion);  // 设置流的版本（可以根据实际情况设置，通常用于处理跨版本兼容性）
        blockInfo.setFile(blockFile);

        writeInfoLog(QString("↳ Successed create block file %1").arg(blockInfo.filePath()));
    }
    else
    {
        writeErrorLog(QString("↳ Can not create block file %1").arg(blockInfo.filePath()));
        QMessageBox::critical(this, "Error", QString("Can not create block file %1").arg(blockInfo.filePath()));
        return;
    }

    /* 获取读取的信息（块大小和算法） */
    const size_t block_size = ui->cbBlockSize->currentText().toInt();  // 每个块的大小
    const HashAlg alg = (HashAlg)ui->cbHashAlg->currentIndex();
    writeInfoLog(QString("↳ Block %1 Bytes, Hash algorithm %2 (index: %3)").arg(QString::number(block_size), ui->cbHashAlg->currentText(), QString::number(alg)));

    ui->lcdNumber->display((int)(sourceInfo.size() / block_size));

    /* 开始读取 -> 计算哈希 -> 写入 */
    QByteArray buf;
    qint64 ptr_loc = 0;  // 读取指针目前所处的位置
    unsigned int cur_block_size = 0;  // 本次读取块的大小（因为文件末尾最后块的大小有可能不是块大小的整数倍）
    QByteArray buf_hash;
    while (!in.atEnd())
    {
        buf = sourceFile.read(block_size);
        cur_block_size = buf.size();  // 计算当前要写入的字节数，防止越界
        ptr_loc += cur_block_size;    // 记录指针位置
        buf_hash = getDataHash(buf, alg);  // 计算哈希
        out << buf_hash;

        writeInfoLog(QString("↳ Read: %1, Hash: %2, Pointer location: %3").arg(buf.toHex(), buf_hash.toHex(), QString::number(ptr_loc)));  // 输出读取的内容（十六进制格式显示）
    }
    blockFile.close();

    /* 解锁按钮 */
    setActivityWidget(true);
}


void MainWindow::selectSourceFile()
{
    QString path = QFileDialog::getOpenFileName(this, "Select file", QDir::homePath());
    if (path.isEmpty())
    {
        QMessageBox::warning(this, "Warning", "Do not selected any file!");
        return;
    }
    ui->leSourceFile->setText(path);

    /* 自动添加 block 文件路径 */
    QFileInfo fileInfo(path);  // 使用 QFileInfo 解析路径
     // QString fileName = fileInfo.fileName(); // 获取源文件文件名
     // QString filePath = fileInfo.path();     // 获取去除文件名后的路径
    QString blockFilePath = fileInfo.filePath().append(".hbk");  // 重新构造文件名 hbk - Hash Block
    ui->leBlockFile->setText(blockFilePath);

    return;
}

void MainWindow::selectBlockFile()
{
    QString path = QFileDialog::getSaveFileName(this, "Save path of block file", QDir::homePath());
    if (path.isEmpty())
    {
        QMessageBox::warning(this, "Warning", "Do not selected any file!");
        return;
    }
    ui->leBlockFile->setText(path);

    return;
}

/** 执行测试
 * @brief MainWindow::runTestModule
 */
void MainWindow::runTestModule()
{

}


