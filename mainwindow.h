#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "DatabaseService.h"

#include <QMainWindow>
#include <QSqlDatabase>


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

private:
    /* 程序保存相关操作 */
    void saveSettings();
    void loadSettings();

    /* 数据库 & 数据表相关操作 */
    void disconnectCurDBandSetUi();

private slots:
    void setActivityWidget(const bool activity);

    void autoConnectionDBModule();
    void testBlockWritePerformanceModule();  // 测试分块写入性能

    void aboutThisProject();
    void selectSourceFile();
    void selectBlockFile();
    void runTestModule();

    void closeEvent(QCloseEvent* event) override;

private:
    Ui::MainWindow* ui;

    DatabaseService* _dbs; // 当前操作的数据库对象
    QString _cur_tb;       // 当前正在操作的表名
    bool _isFinalConnDB;   // 最终成功连接到数据库

};
#endif // MAINWINDOW_H
