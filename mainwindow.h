#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "AsyncComputeModule.h"

#include <QMainWindow>
#include <QThread>

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

    void startTestSegmentationPerformance();  // 测试分块分割（segmentation）写入性能
    void startTestRecoverPerformance();  // 测试分块恢复性能

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

    QThread* _threadAsyncJob;       // 用于计算的线程
    AsyncComputeModule* _asyncJob;  // 并行计算任务

    bool is_db_conn;   // 子线程数据库连接状态
};
#endif // MAINWINDOW_H
