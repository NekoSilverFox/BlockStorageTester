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

    bool createTable(const QString& tbName);
    bool insertNewRow(const QString& tbName, const QByteArray& hashValue, const QString& fileNameconst, int size, const int location);
    int getHashRepeatTimes(const QString& tbName, const QByteArray& hashValue);
    bool updateCounter(const QString& tbName, const QByteArray& hashValue, int count);
    unsigned int getTableRowNumber(const QString& tbName);  // 获取表的行数 TODO

    void closeEvent(QCloseEvent* event) override;

private slots:
    void setActivityWidget(const bool activity);

    void autoConnectionDBModule();
    void testBlockWritePerformanceModule();  // 测试分块写入性能

    void aboutThisProject();
    void runTestModule();
    void selectSourceFile();
    void selectBlockFile();

private:
    Ui::MainWindow* ui;

    DatabaseService* _dbs; // 当前操作的数据库对象
    bool _isFinalConnDB;   // 最终成功连接到数据库

};
#endif // MAINWINDOW_H
