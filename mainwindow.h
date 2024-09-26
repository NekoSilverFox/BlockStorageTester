#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "AsyncComputeModule.h"
#include "ResultComput.h"

#include <QMainWindow>
#include <QThread>
#include <QValueAxis>
#include <QChartView>
#include <QChart>
#include <QSplineSeries>
#include <QScatterSeries>


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
    void startTestSegmentationPerformance();    // 测试分块分割（segmentation）写入性能
    void startTestRecoverPerformance();         // 测试分块恢复性能

    /* 结果展示 & 保存 */
    void addSegmentationResult(const ResultComput& seg_result);
    void addRecoverResult(const ResultComput& recover_result);
    bool saveResultComputToCSV(const QString& filePath);

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
                   QChart* chart, QString chart_tital, QFont chart_tital_font,
                   QSplineSeries* spline, QScatterSeries* scatter,
                   QValueAxis* x, QString x_tital,
                   QValueAxis* y, QString y_tital);

private slots:
    void setActivityWidget(const bool activity);

    void autoConnectionDBModule();
    void aboutThisProject();
    void selectSourceFile();
    void selectBlockFile();
    void selectRecoverFile();

    void closeEvent(QCloseEvent* event) override;

private:
    Ui::MainWindow* ui;

    QThread*                _threadAsyncJob;    // 用于计算的线程
    AsyncComputeModule*     _asyncJob;          // 并行计算任务
    QList<ResultComput>*    _listResultComput;  // 所有任务的计算结果

    bool is_db_conn;   // 子线程数据库连接状态

    /* 绘图区 */
    QChart*     _chart_seg_time;    // 画布 - 分块时间
    QChart*     _chart_recover_time;// 画布 - 恢复时间
    QChart*     _chart_repeat_rate; // 画布 - 哈希重复率

    QSplineSeries*  _spline_seg_time;       // 平滑曲线 - 分块时间
    QSplineSeries*  _spline_recover_time;   // 平滑曲线 - 恢复时间
    QSplineSeries*  _spline_repeat_rate;    // 平滑曲线 - 哈希重复率

    QScatterSeries* _scatter_seg_time;       // 数据点 - 分块时间
    QScatterSeries* _scatter_recover_time;   // 数据点 - 恢复时间
    QScatterSeries* _scatter_repeat_rate;    // 数据点 - 哈希重复率

    QFont*      _font_tital;        // 字体 - 标题

};
#endif // MAINWINDOW_H
