#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "ThemeStyle.h"

#include <QPixmap>
#include <QMessageBox>
#include <QSettings>
#include <QCloseEvent>
#include <QFileDialog>
#include <QProgressBar>
#include <QToolTip>

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
    _source_path = "";
    _listResultComput = new QList<ResultComput>();
    ui->cvSegTime->setVisible(false);
    ui->cvRecoverTime->setVisible(false);

    /* 图表显示 */
#if 0
    _x_seg_time           = nullptr;
    _y_seg_time           = nullptr;
    _spline_seg_time      = nullptr;
    _scatter_seg_time     = nullptr;
    _chart_seg_time       = nullptr;

    _x_recover_time       = nullptr;
    _y_recover_time       = nullptr;
    _spline_recover_time  = nullptr;
    _scatter_recover_time = nullptr;
    _chart_recover_time   = nullptr;
#endif

    _x_seg_recover_time         = nullptr;
    _y_seg_recover_time         = nullptr;
    _spline_seg_time            = nullptr;
    _scatter_seg_time           = nullptr;
    _spline_recover_time        = nullptr;
    _scatter_recover_time       = nullptr;
    _chart_seg_recover_time     = nullptr;

    _x_repeat_rate        = nullptr;
    _y_repeat_rate        = nullptr;
    _spline_repeat_rate   = nullptr;
    _scatter_repeat_rate  = nullptr;
    _chart_repeat_rate    = nullptr;

    _font_tital           = nullptr;

    initAllCharts();

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

    /* 运算结果 */
    QStringList res_list_header;
    res_list_header << "File\npath" << "Hash alg" << "Block\nsize"  << "Total\nblocks"
                    << "Hash\nrecords" << "Repeat\nrecords" << "Repeat\nrate" << "Seg time"
                    << "Recovered\nblocks" << "Recovered\nrate" << "Recover\ntime";
    ui->tbwResult->setColumnCount(res_list_header.size());
    ui->tbwResult->setHorizontalHeaderLabels(res_list_header);
    ui->tbwResult->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    ui->tbwResult->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    ui->tbwResult->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    // ui->tbwResult->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    // ui->tbwResult->horizontalHeader()->setSectionResizeMode(4, QHeaderView::ResizeToContents);
    // ui->tbwResult->horizontalHeader()->setSectionResizeMode(5, QHeaderView::ResizeToContents);
    // ui->tbwResult->horizontalHeader()->setSectionResizeMode(6, QHeaderView::ResizeToContents);
    // ui->tbwResult->horizontalHeader()->setSectionResizeMode(7, QHeaderView::ResizeToContents);
    // ui->tbwResult->horizontalHeader()->setSectionResizeMode(8, QHeaderView::ResizeToContents);
    // ui->tbwResult->horizontalHeader()->setSectionResizeMode(9, QHeaderView::ResizeToContents);
    // ui->tbwResult->horizontalHeader()->setSectionResizeMode(10, QHeaderView::ResizeToContents);

    /* 状态栏显示 */
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
    connect(ui->btnSelectBlockHashFile, &QPushButton::clicked, this, &MainWindow::selectBlockFile);
    connect(ui->btnSelectRecoverFile, &QPushButton::clicked, this, &MainWindow::selectRecoverFile);

    connect(ui->btnRunSingleTest, &QPushButton::clicked, this, &MainWindow::startSingleTest);
    connect(ui->btnRunBenchmarkTest, &QPushButton::clicked, this, &MainWindow::startBenchmarkTest);

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
#if 0
    connect(_asyncJob, &AsyncComputeModule::signalRunTestSegmentationPerformance, _asyncJob, &AsyncComputeModule::runTestSegmentationProfmance);
    connect(_asyncJob, &AsyncComputeModule::signalTestSegmentationPerformanceFinished,
            this, [=](const bool is_succ){ if (is_succ) startTestRecoverPerformance(); }); // 分块成功结束后可以开始自动测试恢复性能
    connect(_asyncJob, &AsyncComputeModule::signalRunTestRecoverProfmance, _asyncJob, &AsyncComputeModule::runTestRecoverProfmance);
#endif
    connect(_asyncJob, &AsyncComputeModule::signalRunSingleTest, _asyncJob, &AsyncComputeModule::runSingleTest);
    connect(_asyncJob, &AsyncComputeModule::signalRunBenchmarkTest, _asyncJob, &AsyncComputeModule::runBenchmarkTest);
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

    connect(_asyncJob, &AsyncComputeModule::signalCurSegmentationResult, this, &MainWindow::addSegmentationResult);
    connect(_asyncJob, &AsyncComputeModule::signalCurRecoverResult, this, &MainWindow::addRecoverResult);

    connect(_asyncJob, &AsyncComputeModule::signalAddPointSegTimeAndRepeateRate, this, &MainWindow::addPointSegTimeAndRepeateRate);
    connect(_asyncJob, &AsyncComputeModule::signalAddPointRecoverTime, this, &MainWindow::addPointRecoverTime);

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
    settings.setValue("leBlockHashFile", ui->leBlockHashFile->text());
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
    ui->leBlockHashFile->setText(settings.value("leBlockHashFile", "").toString());
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

void MainWindow::initAllCharts()
{
    writeInfoLog("Start init all charts");

    /* 图表显示（注意释放顺序） */
#if 0
    if (ui->cvSegTime->chart() != nullptr)
    {
        ui->cvSegTime->setChart(new QChart());   // 必须先置为空（解绑）才能 delete chart，否则会造成程序崩溃！！！
    }
    if (ui->cvRecoverTime->chart() != nullptr)
    {
        ui->cvRecoverTime->setChart(new QChart());
    }
#endif
    if (ui->cvSegAndRecoverTime->chart() != nullptr)
    {
        ui->cvSegAndRecoverTime->setChart(new QChart());
    }
    if (ui->cvRepeatRate->chart() != nullptr)
    {
        ui->cvRepeatRate->setChart(new QChart());
    }

    delete _font_tital    ;     // 字体 - 标题

#if 0
    delete _x_seg_time;
    delete _y_seg_time;
    delete _spline_seg_time     ;    // 平滑曲线 - 分块时间
    delete _scatter_seg_time    ;    // 数据点 - 分块时间
    delete _chart_seg_time      ;    // 画布 - 分块时间

    delete _x_recover_time;
    delete _y_recover_time;
    delete _spline_recover_time ;    // 平滑曲线 - 恢复时间
    delete _scatter_recover_time;    // 数据点 - 恢复时间
    delete _chart_recover_time  ;    // 画布 - 恢复时间
#endif

    delete _x_seg_recover_time         ;
    delete _y_seg_recover_time         ;
    delete _spline_seg_time            ;
    delete _scatter_seg_time           ;
    delete _spline_recover_time        ;
    delete _scatter_recover_time       ;
    delete _chart_seg_recover_time     ;

    delete _x_repeat_rate       ;
    delete _y_repeat_rate       ;
    delete _spline_repeat_rate  ;    // 平滑曲线 - 哈希重复率
    delete _scatter_repeat_rate ;    // 数据点 - 哈希重复率
    delete _chart_repeat_rate   ;    // 画布 - 哈希重复率

#if 0
    _x_seg_time          = new QLogValueAxis();
    _y_seg_time          = new QValueAxis();
    _spline_seg_time     = new QLineSeries();
    _scatter_seg_time    = new QScatterSeries();
    _chart_seg_time      = new QChart();

    _x_recover_time      = new QLogValueAxis();
    _y_recover_time      = new QValueAxis();
    _spline_recover_time = new QLineSeries();
    _scatter_recover_time= new QScatterSeries();
    _chart_recover_time  = new QChart();
#endif

    _x_seg_recover_time         = new QLogValueAxis();
    _y_seg_recover_time         = new QValueAxis();
    _spline_seg_time            = new QLineSeries();
    _scatter_seg_time           = new QScatterSeries();
    _spline_recover_time        = new QLineSeries();
    _scatter_recover_time       = new QScatterSeries();
    _chart_seg_recover_time     = new QChart();

    _x_repeat_rate       = new QLogValueAxis();
    _y_repeat_rate       = new QValueAxis();
    _spline_repeat_rate  = new QLineSeries();
    _scatter_repeat_rate = new QScatterSeries();
    _chart_repeat_rate   = new QChart();

    /* 创建字体并设置字体大小 */
    _font_tital          = new QFont();
    _font_tital->setPointSize(ThemeStyle::FONT_TITLE_SIZE);  // 设置标题字体大小为16
    _font_tital->setBold(ThemeStyle::FONT_TITLE_BOLD);     // 设置加粗

    /* 因为一个 QValueAxis 只能绑定一个 Chart（否则会出错），所以这里使用匿名对象的方式,
     * QCategoryAxis 可以为每个 X 轴的值指定标签，并且它会自动为每个标签绘制对应的轴线（网格线）。
     */
#if 0
    initChart(ui->cvSegTime, _chart_seg_time, "Segmentation time versus block size", *_font_tital,
              _spline_seg_time, _scatter_seg_time,
              _x_seg_time, "Block size (Byte)",
              _y_seg_time, "Time (s)");

    initChart(ui->cvRecoverTime, _chart_recover_time, "Recover time versus block size", *_font_tital,
              _spline_recover_time, _scatter_recover_time,
              _x_recover_time, "Block size (Byte)",
              _y_recover_time, "Time (s)");
#endif

    QString hash_name = Hash::getHashName((HashAlg)ui->cbBenchmarkAlg->currentIndex());

    initChart(ui->cvSegAndRecoverTime, _chart_seg_recover_time,
              QString("Segmentation and Recover time versus block size (Base on %1)").arg(hash_name),
              *_font_tital, true,
              QList<QLineSeries*>() << _spline_seg_time << _spline_recover_time,
              QList<QString>() << "" << "",
              QList<QScatterSeries*>() << _scatter_seg_time << _scatter_recover_time,
              QList<QString>() << "Segmentation time" << "Restoration time",
              QList<Qt::GlobalColor>() << Qt::blue << Qt::green,
              QList<Qt::GlobalColor>() << Qt::red  << Qt::darkYellow,
              _x_seg_recover_time, "Block size (Byte)",
              _y_seg_recover_time, "Time (s)");

    initChart(ui->cvRepeatRate, _chart_repeat_rate,
              QString("Percentage of repeats versus block size (Base on %1)").arg(hash_name),
              *_font_tital, false,
              _spline_repeat_rate, _scatter_repeat_rate,
              _x_repeat_rate, "Block size (Byte)",
              _y_repeat_rate, "Percent (%)");

    writeSuccLog("Successed init all charts");
}

/**
 * @brief MainWindow::initChart  初始化指定图表
 * @param chartView UI空间
 * @param chart 画布
 * @param chart_tital 标题
 * @param chart_tital_font 字体样式
 * @param spline 曲线图对象
 * @param scatter 散点图对象
 * @param x X轴
 * @param x_tital X轴标题
 * @param y Y轴
 * @param y_tital Y轴标题
 */
bool MainWindow::initChart(QChartView* chartView, QChart* chart,
                           QString chart_tital, QFont chart_tital_font, bool legend_visible,
                           QLineSeries* spline, QScatterSeries* scatter,
                           QLogValueAxis* x, QString x_tital,
                           QValueAxis* y, QString y_tital)
{
    writeInfoLog(QString("Start init chart %1").arg(chart_tital));

    if (!chartView) {
        qDebug() << "InitChart failed: chartView is nullptr";
        writeErrorLog("InitChart failed: chartView is nullptr");
        return false;
    }
    if (!chart) {
        qDebug() << "InitChart failed: chart is nullptr";
        writeErrorLog("InitChart failed: chart is nullptr");
        return false;
    }
    if (!spline) {
        qDebug() << "InitChart failed: spline is nullptr";
        writeErrorLog("InitChart failed: spline is nullptr");
        return false;
    }
    if (!scatter) {
        qDebug() << "InitChart failed: scatter is nullptr";
        writeErrorLog("InitChart failed: scatter is nullptr");
        return false;
    }
    if (!x) {
        qDebug() << "InitChart failed: x axis is nullptr";
        writeErrorLog("InitChart failed: x axis is nullptr");
        return false;
    }
    if (!y) {
        qDebug() << "InitChart failed: y axis is nullptr";
        writeErrorLog("InitChart failed: y axis is nullptr");
        return false;
    }

    chart->setTitleFont(chart_tital_font);  // 应用字体设置到图表标题
    chart->setTitle(chart_tital);
    chart->legend()->setVisible(legend_visible);  // 显示图例
    chart->setMargins(QMargins(0, 0, 0, 0));  // 移除图像周围的留白区域

    chart->addAxis(x, Qt::AlignBottom);  // 【！！！注意：需要立刻放入chart！否则可能会出错！！！】
    x->setTitleText(x_tital);
    // x->setLabelsAngle(-45);  // 设置轴上的文字倾斜 45 度
    x->setBase(2);              // 基数为 2 (处理 2, 4, 8, 16...等倍数增长的数据)
    x->setLabelFormat("%d");  // 设置标签格式，显示整数
    x->setRange(4, 2048);

    chart->addAxis(y, Qt::AlignLeft);
    y->setTitleText(y_tital);
    // y->setLabelsAngle(-45);  // 设置轴上的文字倾斜 45 度
    y->setTickCount(11);
    y->setLabelFormat("%.2f");  // 设置显示格式，保留两位小数
    y->setRange(0.0, 100.0);
    QFont axisFont;
    axisFont.setPointSize(15);  // 设置合适的字体大小
    y->setLabelsFont(axisFont);  // 应用字体到 Y 轴

    chart->addSeries(spline);
    spline->setColor(Qt::blue);
    spline->attachAxis(x);
    spline->attachAxis(y);

    chart->addSeries(scatter);
    scatter->setColor(Qt::red);  // 设置曲线颜色
    scatter->setMarkerSize(ThemeStyle::SCATTER_MARK_SIZE);
    scatter->attachAxis(x);
    scatter->attachAxis(y);

    // 当鼠标悬停在点上时，触发信号，显示数据
    QObject::connect(scatter, &QScatterSeries::hovered, this, [=](const QPointF& point, bool state) {
        if (state) {
            QToolTip::showText(QCursor::pos(),
                               QString("X: %1, Y: %2").arg(point.x()).arg(point.y()),
                               chartView);
        }
    });  // 使用 UniqueConnection 避免信号重复连接


    if (chartView->chart() != nullptr)
    {
        chartView->setChart(new QChart());  // 作用是与之前的QChart解绑
    }
    chartView->setChart(chart);
    chartView->setRenderHint(QPainter::Antialiasing);  // 抗锯齿

    writeSuccLog(QString("Successed init chart %1").arg(chart_tital));
    return true;
}

bool MainWindow::initChart(QChartView* chartView, QChart* chart,
                           QString chart_tital, QFont chart_tital_font, bool legend_visible,
                           QList<QLineSeries*>  list_spline,  QList<QString> list_spline_title,
                           QList<QScatterSeries*> list_scatter, QList<QString> list_scatter_title,
                           QList<Qt::GlobalColor> list_spline_color,  QList<Qt::GlobalColor> list_scatter_color,
                           QLogValueAxis* x, QString x_tital,
                           QValueAxis* y,    QString y_tital)
{
    writeInfoLog(QString("Start init chart list %1").arg(chart_tital));

    if (list_spline.size() != list_scatter.size())
    {
        qDebug() << "InitChart failed: Size of spline list != size of scatter list";
        writeErrorLog("InitChart failed: Size of spline list != size of scatter list");
        return false;
    }

    if (list_spline.size() != list_spline_title.size())
    {
        qDebug() << "InitChart failed: Size of spline list != size of spline title list";
        writeErrorLog("InitChart failed: Size of spline list != size of spline title list");
        return false;
    }

    if (list_scatter.size() != list_scatter_title.size())
    {
        qDebug() << "InitChart failed: Size of scatter list != size of scatter title list";
        writeErrorLog("InitChart failed: Size of scatter list != size of scatter title list");
        return false;
    }

    if (list_spline_color.size() != list_scatter_color.size())
    {
        qDebug() << "InitChart failed: Size of spline color != size of spline color";
        writeErrorLog("InitChart failed: Size of spline color != size of spline color");
        return false;
    }

    if (!chartView) {
        qDebug() << "InitChart failed: chartView is nullptr";
        writeErrorLog("InitChart failed: chartView is nullptr");
        return false;
    }
    if (!chart) {
        qDebug() << "InitChart failed: chart is nullptr";
        writeErrorLog("InitChart failed: chart is nullptr");
        return false;
    }
    if (!x) {
        qDebug() << "InitChart failed: x axis is nullptr";
        writeErrorLog("InitChart failed: x axis is nullptr");
        return false;
    }
    if (!y) {
        qDebug() << "InitChart failed: y axis is nullptr";
        writeErrorLog("InitChart failed: y axis is nullptr");
        return false;
    }

    chart->setTitleFont(chart_tital_font);  // 应用字体设置到图表标题
    chart->setTitle(chart_tital);
    chart->legend()->setVisible(legend_visible);  // 显示图例
    chart->setMargins(QMargins(0, 0, 0, 0));  // 移除图像周围的留白区域

    chart->addAxis(x, Qt::AlignBottom);  // 【！！！注意：需要立刻放入chart！否则可能会出错！！！】
    x->setTitleText(x_tital);
    // x->setLabelsAngle(-45);  // 设置轴上的文字倾斜 45 度
    x->setBase(2);              // 基数为 2 (处理 2, 4, 8, 16...等倍数增长的数据)
    x->setLabelFormat("%d");  // 设置标签格式，显示整数
    x->setRange(4, 2048);

    chart->addAxis(y, Qt::AlignLeft);
    y->setTitleText(y_tital);
    // y->setLabelsAngle(-45);  // 设置轴上的文字倾斜 45 度
    y->setTickCount(11);
    y->setLabelFormat("%.2f");  // 设置显示格式，保留两位小数
    y->setRange(0.0, 1.0);

    QFont axisFont;
    axisFont.setPointSize(15);  // 设置合适的字体大小
    y->setLabelsFont(axisFont);  // 应用字体到 Y 轴

    for (int i = 0; i < list_spline.size(); ++i)
    {
        QLineSeries*  spline  = list_spline.at(i);
        QScatterSeries* scatter = list_scatter.at(i);
        QString         spline_legend  = list_spline_title.at(i);
        QString         scatter_legend = list_scatter_title.at(i);
        Qt::GlobalColor spline_color   = list_spline_color.at(i);
        Qt::GlobalColor scatter_color  = list_scatter_color.at(i);

        if (!spline)
        {
            qDebug() << "InitChart failed: spline is nullptr";
            writeErrorLog("InitChart failed: spline is nullptr");
            return false;
        }

        if (!scatter)
        {
            qDebug() << "InitChart failed: scatter is nullptr";
            writeErrorLog("InitChart failed: scatter is nullptr");
            return false;
        }

        chart->addSeries(spline);
        spline->setName(spline_legend);
        spline->setColor(spline_color);
        spline->attachAxis(x);
        spline->attachAxis(y);

        chart->addSeries(scatter);
        scatter->setName(scatter_legend);
        scatter->setColor(scatter_color);  // 设置曲线颜色
        scatter->setMarkerSize(ThemeStyle::SCATTER_MARK_SIZE);
        scatter->attachAxis(x);
        scatter->attachAxis(y);

        // 当鼠标悬停在点上时，触发信号，显示数据
        QObject::connect(scatter, &QScatterSeries::hovered, this, [=](const QPointF& point, bool state) {
            if (state) {
                QToolTip::showText(QCursor::pos(),
                                   QString("X: %1, Y: %2").arg(point.x()).arg(point.y()),
                                   chartView);
            }
        });  // 使用 UniqueConnection 避免信号重复连接
    }

    if (chartView->chart() != nullptr)
    {
        chartView->setChart(new QChart());
    }
    chartView->setChart(chart);
    chartView->setRenderHint(QPainter::Antialiasing);  // 抗锯齿

    writeSuccLog(QString("Successed init chart %1").arg(chart_tital));
    return true;
}

/**
 * @brief MainWindow::addPointSegTimeAndRepeateRate 在绘图区添加分割时间和重复率的点
 * @param recover_result 计算结果
 */
bool MainWindow::addPointSegTimeAndRepeateRate(const ResultComput &result)
{
#if 0
    if (!_chart_seg_time) {
        qDebug() << "Add data point failed: chart seg time is nullptr";
        writeErrorLog("Add data point failed: chart seg time is nullptr");
        return false;
    }
#endif
    if (!_chart_seg_recover_time) {
        qDebug() << "Add data point failed: chart seg and recover time is nullptr";
        writeErrorLog("Add data point failed: chart seg and recover time is nullptr");
        return false;
    }
    if (!_spline_seg_time) {
        qDebug() << "Add data point failed: spline seg time is nullptr";
        writeErrorLog("Add data point failed: spline seg time is nullptr");
        return false;
    }
    if (!_scatter_seg_time) {
        qDebug() << "Add data point failed: scatter seg time is nullptr";
        writeErrorLog("Add data point failed: scatter seg time is nullptr");
        return false;
    };
    if (!_chart_repeat_rate) {
        qDebug() << "Add data point failed: chart repeat rate is nullptr";
        writeErrorLog("Add data point failed: chart repeat rate is nullptr");
        return false;
    }
    if (!_spline_repeat_rate) {
        qDebug() << "Add data point failed: spline repeat rate is nullptr";
        writeErrorLog("Add data point failed: spline repeat rate is nullptr");
        return false;
    }
    if (!_scatter_repeat_rate) {
        qDebug() << "Add data point failed: scatter repeat rate is nullptr";
        writeErrorLog("Add data point failed: scatter repeat rate is nullptr");
        return false;
    }

    writeInfoLog(QString("Add data point (%1, %2) to chart Seg time").arg(QString::number(result.blockSize), QString::number(result.segTime)));
    _spline_seg_time->append( result.blockSize, result.segTime);
    _scatter_seg_time->append(result.blockSize, result.segTime);

    if (result.segTime > _y_seg_recover_time->max())
    {
        _y_seg_recover_time->setRange(0.0, result.segTime);
    }

    writeInfoLog(QString("Add data point (%1, %2) to chart Repeate rate").arg(QString::number(result.blockSize), QString::number(result.repeatRate)));
    _spline_repeat_rate->append( result.blockSize, result.repeatRate);
    _scatter_repeat_rate->append(result.blockSize, result.repeatRate);

    if (result.repeatRate > _y_repeat_rate->max())
    {
        _y_repeat_rate->setRange(0.0, result.repeatRate);
    }

    return true;
}

/**
 * @brief MainWindow::addPointRecoverTime 在绘图区添加恢复时间的点
 * @param recover_result 计算结果
 */
bool MainWindow::addPointRecoverTime(const ResultComput &result)
{
#if 0
    if (!_chart_recover_time) {
        qDebug() << "Add data point failed: chart recover time is nullptr";
        writeErrorLog("Add data point failed: chart recover time is nullptr");
        return false;
    }
#endif
    if (!_chart_seg_recover_time) {
        qDebug() << "Add data point failed: chart seg and recover time is nullptr";
        writeErrorLog("Add data point failed: chart seg and recover time is nullptr");
        return false;
    }
    if (!_spline_recover_time) {
        qDebug() << "Add data point failed: spline recover time is nullptr";
        writeErrorLog("Add data point failed: spline recover time is nullptr");
        return false;
    }
    if (!_scatter_recover_time) {
        qDebug() << "Add data point failed: scatter recover time is nullptr";
        writeErrorLog("Add data point failed: scatter recover time is nullptr");
        return false;
    };

    writeInfoLog(QString("Add data point (%1, %2) to chart Recovered Time").arg(QString::number(result.blockSize), QString::number(result.recoveredTime)));
    _spline_recover_time->append( result.blockSize, result.recoveredTime);
    _scatter_recover_time->append(result.blockSize, result.recoveredTime);

    if (result.recoveredTime > _y_seg_recover_time->max())
    {
        _y_seg_recover_time->setRange(0.0, result.recoveredTime);
    }

    return true;
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
    ui->leBlockHashFile->setReadOnly(!activity);
    ui->leRecoverFile->setReadOnly(!activity);
    ui->btnSelectSourceFile->setEnabled(activity);
    ui->btnSelectBlockHashFile->setEnabled(activity);
    ui->btnSelectRecoverFile->setEnabled(activity);

    ui->cbBlockSize->setEnabled(activity);
    ui->cbHashAlg->setEnabled(activity);
    ui->btnRunSingleTest->setEnabled(activity);

    ui->cbBenchmarkAlg->setEnabled(activity);
    ui->btnRunBenchmarkTest->setEnabled(activity);
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

#if 0
/**
 * @brief MainWindow::startTestSegmentationPerformance 测试分块分割性能（会发送信号让子线程实际的去执行计算任务）
 */
void MainWindow::startTestSegmentationPerformance()
{
    setActivityWidget(false);
    writeInfoLog("Start Test Segmentation Performance");
    /* 没有选择文件或保存路径 */
    if (ui->leSourceFile->text().isEmpty() || ui->leBlockHashFile->text().isEmpty())
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
    emit _asyncJob->signalRunTestSegmentationPerformance(ui->leSourceFile->text(), ui->leBlockHashFile->text(), alg, block_size);
}

/**
 * @brief MainWindow::startTestRecoverPerformance 测试分块恢复性能（会发送信号让子线程实际的去执行计算任务）
 */
void MainWindow::startTestRecoverPerformance()
{
    setActivityWidget(false);
    writeInfoLog("Start Test Recover Performance");
    /* 没有选择文件或保存路径 */
    if (ui->leBlockHashFile->text().isEmpty() || ui->leRecoverFile->text().isEmpty())
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
    emit _asyncJob->signalRunTestRecoverProfmance(ui->leRecoverFile->text(), ui->leBlockHashFile->text(), alg, block_size);
}
#endif

/**
 * @brief MainWindow::startSingleTest 执行单步测试（分块性能 + 恢复性能）
 */
void MainWindow::startSingleTest()
{
    if (ui->leSourceFile->text().isEmpty())
    {
        writeErrorLog("Source file path is empty");
        QMessageBox::warning(this, "Warning", "Source file path is empty!");
        setActivityWidget(true);
        return;
    }
    if (ui->leBlockHashFile->text().isEmpty())
    {
        writeErrorLog("Block file path is empty");
        QMessageBox::warning(this, "Warning", "Block file path is empty!");
        setActivityWidget(true);
        return;
    }
    if (ui->leRecoverFile->text().isEmpty())
    {
        writeErrorLog("Recover file path is empty");
        QMessageBox::warning(this, "Warning", "Recover file path is empty!");
        setActivityWidget(true);
        return;
    }

    /* 获取读取的信息（块大小和算法） */
    _source_path = ui->leSourceFile->text();
    const size_t block_size = ui->cbBlockSize->currentText().toInt();  // 每个块的大小(Byte)
    const HashAlg alg = HashAlg(ui->cbHashAlg->currentIndex());

    writeInfoLog(QString("Block %1 Bytes, Hash algorithm %2 (index: %3)").arg(QString::number(block_size), ui->cbHashAlg->currentText(), QString::number(alg)));

    emit _asyncJob->signalRunSingleTest(ui->leSourceFile->text(),
                                        ui->leBlockHashFile->text(),
                                        ui->leRecoverFile->text(),
                                        alg, block_size);
}

/**
 * @brief MainWindow::startBenchmarkTest 自动基准测试
 */
void MainWindow::startBenchmarkTest()
{
    setActivityWidget(false);
    writeInfoLog("Start Benchmark Test");

    if (ui->leSourceFile->text().isEmpty())
    {
        writeErrorLog("Source file path is empty");
        QMessageBox::warning(this, "Warning", "Source file path is empty!");
        setActivityWidget(true);
        return;
    }
    if (ui->leBlockHashFile->text().isEmpty())
    {
        writeErrorLog("Block file path is empty");
        QMessageBox::warning(this, "Warning", "Block file path is empty!");
        setActivityWidget(true);
        return;
    }
    if (ui->leRecoverFile->text().isEmpty())
    {
        writeErrorLog("Recover file path is empty");
        QMessageBox::warning(this, "Warning", "Recover file path is empty!");
        setActivityWidget(true);
        return;
    }

    initAllCharts();

    _source_path = ui->leSourceFile->text();
    const HashAlg alg = HashAlg(ui->cbBenchmarkAlg->currentIndex());
    QList<size_t> blockSizeList;  // 参与测试的所有块大小
    for (int i = 0; i < ui->cbBlockSize->count(); ++i)
    {
        blockSizeList.append(ui->cbBlockSize->itemText(i).toUInt());
        // qDebug() << ui->cbBlockSize->itemText(i).toUInt();
    }

    emit _asyncJob->signalRunBenchmarkTest(ui->leSourceFile->text(),
                                           ui->leBlockHashFile->text(),
                                           ui->leRecoverFile->text(),
                                           alg, blockSizeList);
}

/**
 * @brief MainWindow::addSegmentationResult 将分块测试结果写入到表格中
 * @param seg_result 分块测试结果
 */
void MainWindow::addSegmentationResult(const ResultComput& seg_result)
{
    /* 增加 新行 */
    const size_t i_row = ui->tbwResult->rowCount();
    ui->tbwResult->insertRow(i_row);

    /* 填充新行的每一列数据 */
    ui->tbwResult->setItem(i_row, 0, new QTableWidgetItem(seg_result.sourceFilePath));
    ui->tbwResult->setItem(i_row, 1, new QTableWidgetItem(Hash::getHashName(seg_result.hashAlg)));
    ui->tbwResult->setItem(i_row, 2, new QTableWidgetItem(QString::number(seg_result.blockSize)));
    ui->tbwResult->setItem(i_row, 3, new QTableWidgetItem(QString::number(seg_result.totalBlock)));
    ui->tbwResult->setItem(i_row, 4, new QTableWidgetItem(QString::number(seg_result.hashRecordDB)));
    ui->tbwResult->setItem(i_row, 5, new QTableWidgetItem(QString::number(seg_result.repeatRecord)));
    ui->tbwResult->setItem(i_row, 6, new QTableWidgetItem(QString::number(seg_result.repeatRate, 'f', 2).append('%')));
    ui->tbwResult->setItem(i_row, 7, new QTableWidgetItem(QString::number(seg_result.segTime, 'f', 2).append('s')));

    /* 设置焦点到新增行的第一列，并选中整行 */
    ui->tbwResult->setCurrentCell(i_row, 0);
    ui->tbwResult->selectRow(i_row); // 选中整行
}

/**
 * @brief MainWindow::addRecoverResult 将恢复结果写入到表格中
 * @param recover_result 恢复结果
 */
void MainWindow::addRecoverResult(const ResultComput& recover_result)
{
    /* 定位到最后一行 */
    const size_t i_row = ui->tbwResult->rowCount() - 1;

    /* 从中间部分开始填充之前没补充的每一列数据 */
    ui->tbwResult->setItem(i_row, 8,  new QTableWidgetItem(QString::number(recover_result.recoveredBlock)));
    ui->tbwResult->setItem(i_row, 9,  new QTableWidgetItem(QString::number(recover_result.recoveredRate, 'f', 2).append('%')));
    ui->tbwResult->setItem(i_row, 10, new QTableWidgetItem(QString::number(recover_result.recoveredTime, 'f', 2).append('s')));

    /* 设置焦点到新增行的第一列，并选中整行 */
    ui->tbwResult->setCurrentCell(i_row, 0);
    ui->tbwResult->selectRow(i_row); // 选中整行

    /* 保存整条运算结果 */
    _listResultComput->append(recover_result);
}

/**
 * @brief MainWindow::saveResultComputToCSV 所有运算结果保存为 CSV
 * @return
 */
bool MainWindow::saveResultComputToCSV()
{
    QFileInfo source_info(_source_path);

    QString csv_path = source_info.dir().filePath(QString("RESULT_").append(source_info.baseName().append(".csv")));

    QFile file(csv_path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        qDebug() << "Unable to open file for writing:" << file.errorString();
        return false;
    }

    QTextStream out(&file);
    out << "sourceFilePath,hashAlg,blockSize,"
           "totalBlock,hashRecordDB,repeatRecord,repeatRate,segTime,"
           "recoveredBlock,recoveredRate,recoveredTime"
           "\n";

    /* 遍历 QList<ResultComput>，将每个 ResultComput 写入一行 CSV  */
    for (const ResultComput& result : *_listResultComput)
    {
        out << result.sourceFilePath << ','  // 源文件路径
            << Hash::getHashName(result.hashAlg)  << ','  // 分块/恢复所用的哈希函数
            << result.blockSize      << ','  // 分块大小
            << result.totalBlock     << ','  // 源文件被分成了多少块
            << result.hashRecordDB   << ','  // 数据库数据表中中记录的哈希条数
            << result.repeatRecord   << ','  // 重复的哈希值/块数量
            << result.repeatRate     << ','  // 重复率
            << result.segTime        << ','  // 分块任务所用的时间
            << result.recoveredBlock << ','  // 成功恢复的块数量
            << result.recoveredRate  << ','  // 恢复率
            << result.recoveredTime  << "\n";// 恢复任务所用时间
    }

    qDebug() << "Successed save result compute to path:" << csv_path;
    file.close();
    return true;
}

bool MainWindow::saveLog()
{
    QFileInfo source_info(_source_path);

    QString log_path = source_info.dir().filePath(QString("LOG_").append(source_info.baseName().append(".log")));

    QFile file(log_path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        qDebug() << "Unable to open file for writing:" << file.errorString();
        return false;
    }

    QTextStream out(&file);
    out << ui->txbLog->toPlainText();

    qDebug() << "Successed save log to path:" << log_path;
    file.close();
    return true;
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
    QString blockFilePath = sourceInfo.dir().filePath(sourceInfo.baseName().append(".bkh"));;  // 重新构造文件名 bkh - Hash Block
    ui->leBlockHashFile->setText(blockFilePath);

    /* 自动添加 recover 文件路径 */
    QString recoverFilePath = sourceInfo.dir().filePath(QString("RECOVERED_").append(sourceInfo.fileName()));
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
    ui->leBlockHashFile->setText(path);

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

void MainWindow::saveChartAsImage()
{
    // 打开文件对话框，选择保存图片的路径
    QString fileName = QFileDialog::getSaveFileName(this, "Save Chart", QDir::homePath(), "PNG Files (*.png);;JPEG Files (*.jpg);;All Files (*)");

    if (!fileName.isEmpty())
    {
        // 捕获 QChartView 内容并保存为图片
        QPixmap pixmap = this->grab();  // 获取当前视图内容
        if (!pixmap.save(fileName)) {
            QMessageBox::warning(this, "Save Error", "Failed to save the chart as an image.");
        }
    }
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
#if 0
    delete _x_seg_time;
    delete _y_seg_time;

    delete _x_recover_time;
    delete _y_recover_time;
#endif
    delete _x_seg_recover_time;
    delete _y_seg_recover_time;

    delete _x_repeat_rate;
    delete _y_repeat_rate;

    delete _spline_seg_time     ;    // 平滑曲线 - 分块时间
    delete _spline_recover_time ;    // 平滑曲线 - 恢复时间
    delete _spline_repeat_rate  ;    // 平滑曲线 - 哈希重复率

    delete _scatter_seg_time    ;    // 数据点 - 分块时间
    delete _scatter_recover_time;    // 数据点 - 恢复时间
    delete _scatter_repeat_rate ;    // 数据点 - 哈希重复率

    /* 注意，画布最后再删除，因为上面图表对象是chart的子对象 */
#if 0
    delete _chart_seg_time      ;    // 画布 - 分块时间
    delete _chart_recover_time  ;    // 画布 - 恢复时间
#endif
    delete _chart_seg_recover_time;
    delete _chart_repeat_rate   ;    // 画布 - 哈希重复率

    delete _font_tital    ;     // 字体 - 标题

    /* 自动保存运算结果到 CSV */
    if (!_source_path.isEmpty())
    {
        saveResultComputToCSV();
        saveLog();
    }
    delete _listResultComput;

    saveSettings();
    event->accept();
    QMainWindow::closeEvent(event);
}

/**
 * @brief MainWindow::contextMenuEvent 重写右键菜单事件
 * @param event
 */
void MainWindow::contextMenuEvent(QContextMenuEvent *event)
{
    QMenu contextMenu(this);

    // 添加保存图片的选项
    QAction saveAsImageAction("Save as Image", this);
    connect(&saveAsImageAction, &QAction::triggered, this, &MainWindow::saveChartAsImage);

    // 将动作添加到上下文菜单中
    contextMenu.addAction(&saveAsImageAction);

    // 显示上下文菜单
    contextMenu.exec(event->globalPos());
}
