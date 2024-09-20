#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "HashAlgorithm.h"
#include "InputFile.h"
#include "ThemeStyle.h"

#include <QPixmap>
#include <QMessageBox>
#include <QSettings>
#include <QCloseEvent>
#include <QFileDialog>
#include <QTimer>

#include <QSqlQuery>
#include <QSqlError>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    /* 初始化 */
    writeInfoLog("Start init");
    writeInfoLog(QString("Available drivers: %1").arg(QSqlDatabase::drivers().join(" ")));

    setWindowIcon(QIcon(":/icons/logo.png"));
    ui->lbPicSQLServer->setPixmap(QPixmap(":/icons/sql-server.png"));
    ui->lbPicSQLServer->setScaledContents(true);

    ui->lbDBConnected->setStyleSheet(ThemeStyle::LABLE_RED);

    _cur_tb = "";
    is_db_conn = false;

    QLabel* lbRuningTestType = new QLabel(this);
    lbRuningTestType->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);  // 左对齐并垂直居中
    lbRuningTestType->setContentsMargins(12, 0, 0, 0);  // 布局边缘的空间
    lbRuningTestType->setText("Running test Type:");
    ui->statusbar->addWidget(lbRuningTestType);

    QProgressBar* progressBar = new QProgressBar(this);
    progressBar->setRange(0, 100);
    progressBar->setValue(0);
    progressBar->setTextVisible(true);
    ui->statusbar->addPermanentWidget(progressBar); // 永久部件添加到状态栏

    QLabel *lbGithub = new QLabel(this);
    lbGithub->setFrameStyle(QFrame::Box | QFrame::Sunken);
    lbGithub->setText(tr("<a href=\"https://github.com/NekoSilverFox/BlockStorageTester\">GitHub</a>"));
    lbGithub->setOpenExternalLinks(true);
    ui->statusbar->addPermanentWidget(lbGithub); // 永久部件添加到状态栏


    /* 连接信号和槽 */
    connect(ui->actionAbout, &QAction::triggered, this, &MainWindow::aboutThisProject);
    connect(ui->btnConnectDB, &QPushButton::clicked, this, &MainWindow::autoConnectionDBModule);
    connect(ui->btnDisconnect, &QPushButton::clicked, this, [=](){emit _asyncJob->signalDisconnDb();});

    connect(ui->btnSelectSourceFile, &QPushButton::clicked, this, &MainWindow::selectSourceFile);
    connect(ui->btnSelectBlockFile, &QPushButton::clicked, this, &MainWindow::selectBlockFile);

    connect(ui->btnRunTest, &QPushButton::clicked, this, [=](){ /// TODO
    });

    connect(this, &MainWindow::signalSetActivityWidget, this, &MainWindow::setActivityWidget);

    /* 接收/处理子线程任务发出的信号 */
    _threadAsyncJob = new QThread(this);
    _asyncJob = new AsyncComputeModule();  // 一定不能给子线程任务增加 父对象！！
    _asyncJob->moveToThread(_threadAsyncJob);
    _threadAsyncJob->start();  // 通过 start() 被启动后，它通常处于等待操作系统调度的状态

    connect(_asyncJob, &AsyncComputeModule::signalConnDb, _asyncJob, &AsyncComputeModule::connectDatabase);
    connect(_asyncJob, &AsyncComputeModule::signalDisconnDb, _asyncJob, &AsyncComputeModule::disconnectCurrentDatabase);
    connect(_asyncJob, &AsyncComputeModule::signalDbConnState, this, &MainWindow::asyncJobDbConnStateChanged);
    connect(_asyncJob, &AsyncComputeModule::signalDropCurDb, _asyncJob, &AsyncComputeModule::dropCurrentDatabase);

    connect(_asyncJob, &AsyncComputeModule::signalWriteInfoLog, this, &MainWindow::writeInfoLog);
    connect(_asyncJob, &AsyncComputeModule::signalWriteWarningLog, this, &MainWindow::writeWarningLog);
    connect(_asyncJob, &AsyncComputeModule::signalWriteErrorLog, this, &MainWindow::writeErrorLog);
    connect(_asyncJob, &AsyncComputeModule::signalWriteSuccLog, this, &MainWindow::writeSuccLog);

    connect(_asyncJob, &AsyncComputeModule::signalInfoBox, this, &MainWindow::showInfoBox);
    connect(_asyncJob, &AsyncComputeModule::signalWarnBox, this, &MainWindow::showWarnBox);
    connect(_asyncJob, &AsyncComputeModule::signalErrorBox, this, &MainWindow::showErrorBox);

    connect(_asyncJob, &AsyncComputeModule::signalSetLbDBConnectedStyle, this, &MainWindow::setLbDBConnectedStyle);
    connect(_asyncJob, &AsyncComputeModule::signalFinishJob, _asyncJob, &AsyncComputeModule::finishJob);

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

    writeSuccLog("Successed load settings");
}

/**
 * @brief MainWindow::asyncJobDbConnStateChange 子线程数据库连接状态发生改变
 * @param is_conn
 */
void MainWindow::asyncJobDbConnStateChanged(const bool is_conn)
{
    is_db_conn = is_conn;
    if (is_conn)
    {
        ui->lbDBConnected->setStyleSheet(ThemeStyle::LABLE_GREEN);
    }
    else
    {
        ui->lbDBConnected->setStyleSheet(ThemeStyle::LABLE_RED);
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

void MainWindow::writeSuccLog(const QString& msg)
{
    ui->txbLog->setTextColor(Qt::darkGreen);
    ui->txbLog->append(QString("[INFO] %1").arg(msg));
}

/** 设置小部件的可操作性
 * @brief MainWindow::setActivityWidget
 * @param activity true:启用； false:禁用
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




void MainWindow::aboutThisProject()
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


/**
 * @brief MainWindow::autoConnectionDBModule 模块：自动连接并创建数据库
 */
void MainWindow::autoConnectionDBModule()
{
    /**
     *  【主线程】参数检查
     */
    if (   ui->leHost->text().isEmpty()
        || ui->lePort->text().isEmpty()
        || ui->leDriver->text().isEmpty()
        || ui->leUser->text().isEmpty()
        || ui->lePassword->text().isEmpty()
        || ui->leDatabase->text().isEmpty())
    {
        writeErrorLog("Can not connect to database, do not have enough argument");
        QMessageBox::critical(this, "Error", "Please input all info about the database!");
        return;
    }

    QString host     = ui->leHost->text();
    qint16  port     = ui->lePort->text().toInt();
    QString driver   = ui->leDriver->text();
    QString user     = ui->leUser->text();
    QString password = ui->lePassword->text();
    QString dbName = ui->leDatabase->text().toLower();  // PostgreSQL 数据库只能小写

    /**
     * 【主线程】链接默认数据库，查看指定数据库是否存在
     */
    DatabaseService tmp_dbs; // 当前【主线程】操作的数据库对象，用于实现创建指定数据库
    bool isSucc = tmp_dbs.connectDatabase(host, port,driver, user, password, DEFAULT_DB_CONN);
    if (isSucc)
    {
        ui->lbDBConnected->setStyleSheet(ThemeStyle::LABLE_GREEN);
        writeSuccLog(tmp_dbs.lastLog());
    }
    else
    {
        ui->lbDBConnected->setStyleSheet(ThemeStyle::LABLE_RED);
        writeErrorLog(tmp_dbs.lastLog());
        QMessageBox::warning(this, "Database Connect failed", tmp_dbs.lastLog());
        return;
    }

    /**
     * 【主线程】检查用户指定的数据库是否存在
     */
    if (!tmp_dbs.isDatabaseExist(dbName))
    {
        /* 数据库成功连接 && 数据库不存在 */
        int ret = QMessageBox::question(this, "Automatic create database",
                                        QString("Database `%1` do not exist, do you want automatic create now?").arg(dbName),
                                        QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes);

        /* 自动创建数据库 */
        if (QMessageBox::Yes == ret)
        {
            writeInfoLog(QString("Start to create database %1").arg(dbName));

            bool succ = tmp_dbs.createDatabase(dbName);
            if (succ)
            {
                writeSuccLog(tmp_dbs.lastLog());
            }
            else
            {
                writeErrorLog(tmp_dbs.lastLog());
                QMessageBox::critical(this, "Error", tmp_dbs.lastLog());
                ui->lbDBConnected->setStyleSheet(ThemeStyle::LABLE_RED);
                return;
            }
        }
        else
        {
            writeWarningLog(QString("Automatic create database `%1` cancel").arg(dbName));
            ui->lbDBConnected->setStyleSheet(ThemeStyle::LABLE_RED);
            return;
        }
    }

    /**
     *  【主线程】断开数据库连接，准备交由子线程操作
     */
    isSucc = tmp_dbs.disconnectCurDatabase();
    if (!isSucc)
    {
        writeErrorLog(tmp_dbs.lastLog());
        return;
    }

    /**
     * 【子线程】重新连接为用户指定的数据库
     */
    writeInfoLog(QString("Child thread Re-connect to database `%1` ...").arg(dbName));
    ui->lbDBConnected->setStyleSheet(ThemeStyle::LABLE_ORANGE);
    emit _asyncJob->signalConnDb(host, port, driver, user, password, dbName);
}


void MainWindow::testBlockWritePerformanceModule()
{
#if 0
    emit signalWriteInfoLog("Start test block write performance");

    /* 数据库无连接 */
    if (!is_db_conn)
    {
        emit signalWriteErrorLog("↳ Database do not connected, test end");
        QMessageBox::warning(this, "Warning", "Database do not connected!");
        emit signalThreadFinished();
        return;
    }

    /* 没有选择文件或保存路径 */
    if (ui->leSourceFile->text().isEmpty() || ui->leBlockFile->text().isEmpty())
    {
        emit signalWriteErrorLog("↳ Source file or block file path is empty");
        QMessageBox::warning(this, "Warning", "Source file or block file path is empty!");
        emit signalThreadFinished();
        return;
    }

    /* 为了避免意外操作，暂时禁用按钮 */
    emit signalSetActivityWidget(false);

    /* 打开源文件（输入） */
    InputFile* fin = new InputFile(this, ui->leSourceFile->text());
    if (!fin->isOpen())
    {
        emit signalWriteErrorLog(fin->lastLog());
        QMessageBox::critical(this, "Error", fin->lastLog());
        emit signalThreadFinished();
        return;
    }
    emit signalWriteSuccLog(fin->lastLog());

    /* 创建块文件（输出） */
    QFile blockFile(ui->leBlockFile->text());
    QFileInfo blockInfo;
    QDataStream out;
    if (blockFile.open(QIODevice::WriteOnly))
    {
        out.setDevice(&blockFile);
        out.setVersion(QDataStream::Qt_DefaultCompiledVersion);  // 设置流的版本（可以根据实际情况设置，通常用于处理跨版本兼容性）
        blockInfo.setFile(blockFile);

        emit signalWriteSuccLog(QString("↳ Successed create Hash-Block %1").arg(blockInfo.filePath()));
    }
    else
    {
        emit signalWriteErrorLog(QString("↳ Can not create Hash-Block file %1").arg(blockInfo.filePath()));
        QMessageBox::critical(this, "Error", QString("Can not create Hash-Block file %1").arg(blockInfo.filePath()));
        emit signalThreadFinished();
        return;
    }

    /**
     * 获取读取的信息（块大小和算法）
     */
    const size_t block_size = ui->cbBlockSize->currentText().toInt();  // 每个块的大小
    const HashAlg alg = (HashAlg)ui->cbHashAlg->currentIndex();
    emit signalWriteInfoLog(QString("↳ Block %1 Bytes, Hash algorithm %2 (index: %3)").arg(QString::number(block_size), ui->cbHashAlg->currentText(), QString::number(alg)));

    /**
     * 创建表
     */
    _cur_tb = QString("tb_%1bytes_%2").arg(QString::number(block_size), ui->cbHashAlg->currentText()).toLower();
    emit signalWriteInfoLog(QString("Create table `%1`").arg(_cur_tb));
    bool isSucc = tmp_dbs->createBlockInfoTable(_cur_tb);
    emit signalWriteInfoLog(QString("↳ Run SQL `%1`").arg(tmp_dbs->lastSQL()));
    if (!isSucc)
    {
        emit signalWriteErrorLog(tmp_dbs->lastLog());
        QMessageBox::critical(this, "Error", tmp_dbs->lastLog());
        emit signalThreadFinished();
        return;
    }
    emit signalWriteSuccLog(tmp_dbs->lastLog());
    ui->tbName->setText(_cur_tb);  ///TODO
    ui->lcdNumber->display((int)(fin->fileSize() / block_size));


    /**
     * 开始读取 -> 计算哈希 -> 写入
     */
    QByteArray buf_block;       // 读取的源文件的 buffer（用来暂存当前块）
    qint64 ptr_loc = 0;         // 读取指针目前所处的位置
    unsigned int cur_block_size = 0;  // 本次读取块的大小（因为文件末尾最后块的大小有可能不是块大小的整数倍）
    QByteArray buf_hash;        // 存储当前块的哈希值
    size_t repeat_times = 0;    // 当前块的重复次数
    unsigned int total_repeat_times = 0;


    QTimer timer;
    connect(&timer, &QTimer::timeout, this, [=](){
        emit signalWriteInfoLog(QString("Testing writing performance %1%").arg(ptr_loc / fin->fileSize() * 100));
    });
    timer.start(10);  // 启动定时器，参数为毫秒 ms

    while (!fin->atEnd())
    {
        buf_block = fin->read(block_size);
        cur_block_size = buf_block.size();  // 计算当前读取的字节数，防止越界
        buf_hash = getDataHash(buf_block, alg);  // 计算哈希

        /* 写入数据库 */
        repeat_times = tmp_dbs->getHashRepeatTimes(_cur_tb, buf_hash);
#if !QT_NO_DEBUG
        qDebug() << tmp_dbs->lastLog();
#endif
        if (0 == repeat_times)  // 重复次数为 0 说明没有记录过当前哈希
        {
            tmp_dbs->insertNewBlockInfoRow(_cur_tb, buf_hash,fin->filePath(), ptr_loc, cur_block_size);
        }
        else
        {
            ++total_repeat_times;
            tmp_dbs->updateCounter(_cur_tb, buf_hash, (repeat_times + 1));
            // ui->lcdRepeatTimes->display(QString("%1  Per:%2").arg(total_repeat_times, total_repeat_times / (fin->fileSize() / block_size)));
        }
#if !QT_NO_DEBUG
        qDebug() << tmp_dbs->lastLog();
        qDebug() << QString("↳ Read: %1, Hash: %2, Pointer location: %3, Repet times: %4").arg(
            buf_block.toHex(), buf_hash.toHex(), QString::number(ptr_loc), QString::number(repeat_times));  // 输出读取的内容（十六进制格式显示）
#endif
        out << buf_hash;           // 记录哈希到文件
        ptr_loc += cur_block_size; // 移动指针位置
    }
    blockFile.close();
    emit signalWriteSuccLog("↳ Finish test writing performance");

    /* 解锁按钮 */
    emit signalSetActivityWidget(true);
    emit signalThreadFinished();
#endif
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

/**
 * @brief MainWindow::setLbDBConnectedStyle 设置 lbDBConnected 的风格
 * @param style 风格
 */
void MainWindow::setLbDBConnectedStyle(QString style)
{
    ui->lbDBConnected->setStyleSheet(style);
}

void MainWindow::showInfoBox(const QString& msg)
{
    QMessageBox::information(this, "Info", msg);
}

void MainWindow::showWarnBox(const QString& msg)
{
    QMessageBox::warning(this, "Warning", msg);
}

void MainWindow::showErrorBox(const QString& msg)
{
    QMessageBox::critical(this, "Error", msg);
}

void MainWindow::closeEvent(QCloseEvent* event)
{
    QEventLoop loop;    // 使用事件循环等待任务完成
    connect(_asyncJob, &AsyncComputeModule::signalFinished, &loop, &QEventLoop::quit);

    /* 结束线程并且自动删除使用的数据库 */
    if (is_db_conn)
    {
        emit _asyncJob->signalFinishJob(ui->cbAutoDropDB->isChecked());
        loop.exec();        // 阻塞，直到 signalFinished 被发出
    }
    _threadAsyncJob->quit();
    _threadAsyncJob->wait();
    _threadAsyncJob->deleteLater();

    delete _asyncJob;
    delete _threadAsyncJob;

    saveSettings();
    event->accept();
    QMainWindow::closeEvent(event);
}
