#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "ThemeStyle.h"

#include <QPixmap>
#include <QMessageBox>
#include <QSettings>
#include <QCloseEvent>
#include <QFileDialog>
#include <QProgressBar>

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

    is_db_conn = false;

    ui->tb->setMarkdown("# Hello, World!\n\nThis is **bold** and *italic* text.");
    ui->tb->append("# 123Hello, World!\n\nThis is **bold** and *italic* text.");


    setWindowIcon(QIcon(":/icons/logo.png"));
    ui->lbPicSQLServer->setPixmap(QPixmap(":/icons/sql-server.png"));
    ui->lbPicSQLServer->setScaledContents(true);

    ui->lbDBConnected->setStyleSheet(ThemeStyle::LABLE_RED);
    ui->lbSegmentation->setStyleSheet(ThemeStyle::LABLE_RED);
    ui->lbRecover->setStyleSheet(ThemeStyle::LABLE_RED);

    ui->lcdTotalFileBlocks->display(0);
    ui->lcdTotalDbHashRecords->display(0);
    ui->lcdTotalRepeat->display(0);
    ui->lcdRepeatPercent->display(QString::number(0.0, 'f', 2));
    ui->lcdSegmentationTime->display(QString::number(0.0, 'f', 2));
    ui->lcdNumNeedRecover->display(0);
    ui->lcdTotalUnrecovered->display(0);
    ui->lcdTotalRecovered->display(0);
    ui->lcdRecoveredPercent->display(QString::number(0.0, 'f', 2));
    ui->lcdRecoverTime->display(QString::number(0.0, 'f', 2));

    QLabel* lbRuningJobInfo = new QLabel(this);
    lbRuningJobInfo->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);  // 左对齐并垂直居中
    lbRuningJobInfo->setContentsMargins(12, 0, 0, 0);  // 布局边缘的空间
    lbRuningJobInfo->setText("");
    lbRuningJobInfo->setStyleSheet(ThemeStyle::LABLE_BLUE);
    ui->statusbar->addWidget(lbRuningJobInfo);

    QProgressBar* progressBar = new QProgressBar(this);
    progressBar->setMinimumWidth(200);
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
    connect(ui->btnSelectRecoverFile, &QPushButton::clicked, this, &MainWindow::selectRecoverFile);

    connect(ui->btnRunTest, &QPushButton::clicked, this, &MainWindow::startTestSegmentationPerformance);


    /* 接收/处理子线程任务发出的信号 */
    _threadAsyncJob = new QThread(this);
    _asyncJob = new AsyncComputeModule();  // 一定不能给子线程任务增加 父对象！！
    _asyncJob->moveToThread(_threadAsyncJob);
    _threadAsyncJob->start();  // 通过 start() 被启动后，它通常处于等待操作系统调度的状态
    // _threadAsyncJob->setPriority(QThread::TimeCriticalPriority);

    connect(_asyncJob, &AsyncComputeModule::signalConnDb, _asyncJob, &AsyncComputeModule::connectDatabase);
    connect(_asyncJob, &AsyncComputeModule::signalDisconnDb, _asyncJob, &AsyncComputeModule::disconnectCurrentDatabase);
    connect(_asyncJob, &AsyncComputeModule::signalDbConnState, this, &MainWindow::asyncJobDbConnStateChanged);
    connect(_asyncJob, &AsyncComputeModule::signalDropCurDb, _asyncJob, &AsyncComputeModule::dropCurrentDatabase);

    connect(_asyncJob, &AsyncComputeModule::signalRunTestSegmentationPerformance, _asyncJob, &AsyncComputeModule::runTestSegmentationProfmance);
    connect(_asyncJob, &AsyncComputeModule::signalTestSegmentationPerformanceFinished,
            this, [=](const bool is_succ){ if (is_succ) startTestRecoverPerformance(); }); // 分块成功结束后可以开始自动测试恢复性能
    connect(_asyncJob, &AsyncComputeModule::signalRunTestRecoverProfmance, _asyncJob, &AsyncComputeModule::runTestRecoverProfmance);
    connect(_asyncJob, &AsyncComputeModule::signalFinishAllJob, _asyncJob, &AsyncComputeModule::finishAllJob);


    connect(_asyncJob, &AsyncComputeModule::signalWriteInfoLog, this, &MainWindow::writeInfoLog);
    connect(_asyncJob, &AsyncComputeModule::signalWriteWarningLog, this, &MainWindow::writeWarningLog);
    connect(_asyncJob, &AsyncComputeModule::signalWriteErrorLog, this, &MainWindow::writeErrorLog);
    connect(_asyncJob, &AsyncComputeModule::signalWriteSuccLog, this, &MainWindow::writeSuccLog);

    connect(_asyncJob, &AsyncComputeModule::signalInfoBox, this, &MainWindow::showInfoBox);
    connect(_asyncJob, &AsyncComputeModule::signalWarnBox, this, &MainWindow::showWarnBox);
    connect(_asyncJob, &AsyncComputeModule::signalErrorBox, this, &MainWindow::showErrorBox);

    connect(_asyncJob, &AsyncComputeModule::signalSetLbDBConnectedStyle, this, &MainWindow::setLbDBConnectedStyle);
    connect(_asyncJob, &AsyncComputeModule::signalSetLbSegmentationStyle, this, [=](QString style){ui->lbSegmentation->setStyleSheet(style);});
    connect(_asyncJob, &AsyncComputeModule::signalSetLbRecoverStyle, this, [=](QString style){ui->lbRecover->setStyleSheet(style);});


    connect(_asyncJob, &AsyncComputeModule::signalSetActivityWidget, this, &MainWindow::setActivityWidget);
    connect(_asyncJob, &AsyncComputeModule::signalSetLbRuningJobInfo, this, [=](const QString& info){lbRuningJobInfo->setText(info);});
    connect(_asyncJob, &AsyncComputeModule::signalSetProgressBarValue, this, [=](const int number){progressBar->setValue(number);});
    connect(_asyncJob, &AsyncComputeModule::signalSetProgressBarRange, this, [=](const int minimum, const int maximum){progressBar->setRange(minimum, maximum);});
    connect(_asyncJob, &AsyncComputeModule::signalSetLcdTotalFileBlocks, this, [=](const int number){ui->lcdTotalFileBlocks->display(number);});
    connect(_asyncJob, &AsyncComputeModule::signalSetLcdTotalDbHashRecords, this, [=](const int number){ui->lcdTotalDbHashRecords->display(number);});
    connect(_asyncJob, &AsyncComputeModule::signalSetLcdTotalRepeat, this, [=](const int number){ui->lcdTotalRepeat->display(number);});
    connect(_asyncJob, &AsyncComputeModule::signalSetLcdRepeatPercent, this, [=](const double number){ui->lcdRepeatPercent->display(QString::number(number, 'f', 2));});
    connect(_asyncJob, &AsyncComputeModule::signalSetLcdSegmentationTime, this, [=](const double number){ui->lcdSegmentationTime->display(QString::number(number, 'f', 2));});
    connect(_asyncJob, &AsyncComputeModule::signalSetLcdNumNeedRecover, this, [=](const int number){ui->lcdNumNeedRecover->display(number);});
    connect(_asyncJob, &AsyncComputeModule::signalSetLcdTotalUnrecovered, this, [=](const int number){ui->lcdTotalUnrecovered->display(number);});
    connect(_asyncJob, &AsyncComputeModule::signalSetLcdTotalRecovered, this, [=](const int number){ui->lcdTotalRecovered->display(number);});
    connect(_asyncJob, &AsyncComputeModule::signalSetLcdRecoveredPercent, this, [=](const double number){ui->lcdRecoveredPercent->display(QString::number(number, 'f', 2));});
    connect(_asyncJob, &AsyncComputeModule::signalSetLcdRecoverTime, this, [=](const double number){ui->lcdRecoverTime->display(QString::number(number, 'f', 2));});


    loadSettings();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::saveSettings()
{
    QSettings settings("NekoSilverfox", "BlockStoreTester", this);

    settings.setValue("leHost", ui->leHost->text());
    settings.setValue("lePort", ui->lePort->text());
    settings.setValue("leDriver", ui->leDriver->text());
    settings.setValue("leUser", ui->leUser->text());
    settings.setValue("lePassword", ui->lePassword->text());
    settings.setValue("leDatabase", ui->leDatabase->text());
    settings.setValue("cbAutoDropDB", ui->cbAutoDropDB->isChecked());

    settings.setValue("leSourceFile", ui->leSourceFile->text());
    settings.setValue("leBlockFile", ui->leBlockFile->text());
    settings.setValue("leRecoverFile", ui->leRecoverFile->text());

    settings.setValue("cbBlockSize", ui->cbBlockSize->currentIndex());
    settings.setValue("cbHashAlg", ui->cbHashAlg->currentIndex());

    writeInfoLog("Successed save settings");
}

void MainWindow::loadSettings()
{
    QSettings settings("NekoSilverfox", "BlockStoreTester", this);

    ui->leHost->setText(settings.value("leHost", "localhost").toString());
    ui->lePort->setText(settings.value("lePort", "5432").toString());
    ui->leDriver->setText(settings.value("leDriver", "QPSQL").toString());
    ui->leUser->setText(settings.value("leUser", "").toString());
    ui->lePassword->setText(settings.value("lePassword", "").toString());
    ui->leDatabase->setText(settings.value("leDatabase", "dbBlockStoreTester").toString());
    ui->cbAutoDropDB->setChecked(settings.value("cbAutoDropDB", true).toBool());

    ui->leSourceFile->setText(settings.value("leSourceFile", "").toString());
    ui->leBlockFile->setText(settings.value("leBlockFile", "").toString());
    ui->leRecoverFile->setText(settings.value("leRecoverFile", "").toString());

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

    ui->cbAutoDropDB->setEnabled(activity);

    ui->btnConnectDB->setEnabled(activity);
    ui->btnDisconnect->setEnabled(activity);

    ui->leSourceFile->setReadOnly(!activity);
    ui->leBlockFile->setReadOnly(!activity);
    ui->leRecoverFile->setReadOnly(!activity);
    ui->btnSelectSourceFile->setEnabled(activity);
    ui->btnSelectBlockFile->setEnabled(activity);
    ui->btnSelectRecoverFile->setEnabled(activity);

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

/**
 * @brief MainWindow::startTestSegmentationPerformance 测试分块分割性能（会发送信号让子线程实际的去执行计算任务）
 */
void MainWindow::startTestSegmentationPerformance()
{
    setActivityWidget(false);
    writeInfoLog("Start Test Segmentation Performance");
    /* 没有选择文件或保存路径 */
    if (ui->leSourceFile->text().isEmpty() || ui->leBlockFile->text().isEmpty())
    {
        writeErrorLog("↳ Source file or block file path is empty");
        QMessageBox::warning(this, "Warning", "Source file or block file path is empty!");
        setActivityWidget(true);
        return;
    }

    /* 获取读取的信息（块大小和算法） */
    const size_t block_size = ui->cbBlockSize->currentText().toInt();  // 每个块的大小(Byte)
    const HashAlg alg = HashAlg(ui->cbHashAlg->currentIndex());
    writeInfoLog(QString("Block %1 Bytes, Hash algorithm %2 (index: %3)").arg(QString::number(block_size), ui->cbHashAlg->currentText(), QString::number(alg)));

    /* 发送信号，让子线程去执行计算任务 */
    emit _asyncJob->signalRunTestSegmentationPerformance(ui->leSourceFile->text(), ui->leBlockFile->text(), alg, block_size);
}

/**
 * @brief MainWindow::startTestRecoverPerformance 测试分块恢复性能（会发送信号让子线程实际的去执行计算任务）
 */
void MainWindow::startTestRecoverPerformance()
{
    setActivityWidget(false);
    writeInfoLog("Start Test Recover Performance");
    /* 没有选择文件或保存路径 */
    if (ui->leBlockFile->text().isEmpty() || ui->leRecoverFile->text().isEmpty())
    {
        writeErrorLog("↳ Block file or Recover file path is empty");
        QMessageBox::warning(this, "Warning", "Block file or Recover file path is empty!");
        setActivityWidget(true);
        return;
    }

    /* 获取读取的信息（块大小和算法） */
    const size_t block_size = ui->cbBlockSize->currentText().toInt();  // 每个块的大小(Byte)
    const HashAlg alg = HashAlg(ui->cbHashAlg->currentIndex());
    writeInfoLog(QString("Block %1 Bytes, Hash algorithm %2 (index: %3)").arg(QString::number(block_size), ui->cbHashAlg->currentText(), QString::number(alg)));

    /* 发送信号，让子线程去执行计算任务 */
    emit _asyncJob->signalRunTestRecoverProfmance(ui->leRecoverFile->text(), ui->leBlockFile->text(), alg, block_size);
}


void MainWindow::selectSourceFile()
{
    QString sourceFilePath = QFileDialog::getOpenFileName(this, "Select file", QDir::homePath());
    if (sourceFilePath.isEmpty())
    {
        QMessageBox::warning(this, "Warning", "Do not selected any file!");
        return;
    }
    ui->leSourceFile->setText(sourceFilePath);

    /* 自动添加 block 文件路径 */
    QFileInfo sourceInfo(sourceFilePath);  // 使用 QFileInfo 解析路径
     // QString fileName = fileInfo.fileName(); // 获取源文件文件名
     // QString filePath = fileInfo.path();     // 获取去除文件名后的路径
    QString blockFilePath = sourceInfo.filePath().append(".hbk");  // 重新构造文件名 hbk - Hash Block
    ui->leBlockFile->setText(blockFilePath);

    /* 自动添加 recover 文件路径 */
    QString recoverFilePath = sourceInfo.dir().filePath(QString("RECOVER_").append(sourceInfo.fileName()));
    ui->leRecoverFile->setText(recoverFilePath);

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

void MainWindow::selectRecoverFile()
{
    QString path = QFileDialog::getSaveFileName(this, "Save path of recover file", QDir::homePath());
    if (path.isEmpty())
    {
        QMessageBox::warning(this, "Warning", "Do not selected any file!");
        return;
    }
    ui->leRecoverFile->setText(path);

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
    connect(_asyncJob, &AsyncComputeModule::signalAllJobFinished, &loop, &QEventLoop::quit);

    /* 结束线程并且自动删除使用的数据库 */
    if (is_db_conn)
    {
        emit _asyncJob->signalFinishAllJob(ui->cbAutoDropDB->isChecked());
        loop.exec();        // 阻塞，直到 AsyncComputeModule::signalAllJobFinished 被发出
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
