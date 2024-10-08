#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "AsyncComputeModule.h"
#include "ResultComput.h"

#include <QMainWindow>
#include <QThread>
#include <QValueAxis>
#include <QChartView>
#include <QChart>
#include <QLineSeries>
#include <QScatterSeries>
#include <QCategoryAxis>
#include <QLogValueAxis>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    /* 日志写入相关操作 */
    void writeInfoLog(const QString& msg);
    void writeWarningLog(const QString& msg);
    void writeErrorLog(const QString& msg);
    void writeSuccLog(const QString& msg);

    /* 准备数据并且发送信号执行测试 */
#if 0
    void startTestSegmentationPerformance();    // 测试分块分割（segmentation）写入性能
    void startTestRecoverPerformance();         // 测试分块恢复性能
#endif
    void startSingleTest();                 // 开始单步测试
    void startBenchmarkTest();              // 开始基准测试

    /* 结果展示 & 保存 */
    void addSegmentationResult(const ResultComput& seg_result);
    void addRecoverResult(const ResultComput& recover_result);
    bool saveResultComputToCSV();
    bool saveLog();

    /* 数据库链接指示灯 */
    void setLbDBConnectedStyle(QString style);

    /* 调用对应 QMessageBox */
    void showInfoBox(const QString& msg);
    void showWarnBox(const QString& msg);
    void showErrorBox(const QString& msg);

private:
    /* 程序保存相关操作 */
    void saveSettings();
    void loadSettings();

    /* 数据库 & 数据表相关操作 */
    void asyncJobDbConnStateChanged(const bool is_conn);

    /* 绘图 */
    void initAllCharts();
    bool initChart(QChartView* chartView,
                   QChart* chart, QString chart_tital,
                   QFont chart_tital_font, bool legend_visible,
                   QLineSeries* spline, QScatterSeries* scatter,
                   QLogValueAxis* x, QString x_tital,
                   QValueAxis* y, QString y_tital);

    bool initChart(QChartView* chartView, QChart* chart,
                   QString chart_tital, QFont chart_tital_font, bool legend_visible,
                   QList<QLineSeries*> list_spline, QList<QString> list_spline_title,
                   QList<QScatterSeries*> list_scatter, QList<QString> list_scatter_title,
                   QList<Qt::GlobalColor> list_spline_color,  QList<Qt::GlobalColor> list_scatter_color,
                   QLogValueAxis* x, QString x_tital,
                   QValueAxis* y, QString y_tital);

    bool addPointSegTimeAndRepeateRate(const ResultComput& result);
    bool addPointRecoverTime(const ResultComput& result);

private slots:
    void setActivityWidget(const bool activity);

    void autoConnectionDBModule();
    void aboutThisProject();
    void selectSourceFile();
    void selectUniqueBlockFile();
    void selectBlockHashFile();
    void selectRecoverFile();
    void saveChartAsImage();

    void closeEvent(QCloseEvent* event) override;
    void contextMenuEvent(QContextMenuEvent *event) override;

private:
    Ui::MainWindow* ui;

    QThread*                _threadAsyncJob;    // 用于计算的线程
    AsyncComputeModule*     _asyncJob;          // 并行计算任务
    QList<ResultComput>*    _listResultComput;  // 所有任务的计算结果

    QString                 _source_path;       // 源文件路径（根据这个路径推断 CSV 保存路径和日志路径）

    bool is_db_conn;   // 子线程数据库连接状态

    /* 绘图区 */
#if 0
    QLogValueAxis*  _x_seg_time;
    QValueAxis*     _y_seg_time;

    QLineSeries*  _spline_seg_time;       // 平滑曲线 - 分块时间
    QScatterSeries* _scatter_seg_time;      // 数据点 - 分块时间
    QChart*         _chart_seg_time;        // 画布 - 分块时间

    QLogValueAxis*  _x_recover_time;
    QValueAxis*     _y_recover_time;
    QLineSeries*  _spline_recover_time;   // 平滑曲线 - 恢复时间
    QScatterSeries* _scatter_recover_time;  // 数据点 - 恢复时间
    QChart*         _chart_recover_time;    // 画布 - 恢复时间
#endif

    QLogValueAxis*  _x_seg_recover_time;
    QValueAxis*     _y_seg_recover_time;
    QLineSeries*    _spline_seg_time;       // 平滑曲线 - 分块时间
    QScatterSeries* _scatter_seg_time;      // 数据点 - 分块时间
    QLineSeries*    _spline_recover_time;   // 平滑曲线 - 恢复时间
    QScatterSeries* _scatter_recover_time;  // 数据点 - 恢复时间
    QChart*         _chart_seg_recover_time;// 画布 - 分块时间 & 恢复时间
    QRectF          _orig_rect_chart_seg_rec_time; // 画布范围

    QLogValueAxis*  _x_repeat_rate;
    QValueAxis*     _y_repeat_rate;
    QLineSeries*    _spline_repeat_rate;    // 平滑曲线 - 哈希重复率
    QScatterSeries* _scatter_repeat_rate;   // 数据点 - 哈希重复率
    QChart*         _chart_repeat_rate;     // 画布 - 哈希重复率
    QRectF          _orig_rect_chart_repeat_rate; // 画布范围

    QFont*      _font_tital;                // 字体 - 标题
};
#endif // MAINWINDOW_H
