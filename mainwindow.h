#ifndef MAINWINDOW_H
#define MAINWINDOW_H

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

    void saveSettings();
    void loadSettings();

    bool connectDatabase(const QString& host, const int port, const QString& driver, const QString& user, const QString& pwd, const QString& database);
    bool isDatabaseExist(const QString& db);
    bool createDatabase(const QString& db);
    bool disconnectDatabase();

    /*
     * 数据库中数据表相关操作
     */
    bool createTable(const QString& tbName);
    bool insertNewRow(const QString& tbName, const QByteArray& hashValue, const QString& fileName, const int location);
    int getHashRepeatTimes(const QString& tbName, const QByteArray& hashValue);
    bool updateCounter(const QString& tbName, const QByteArray& hashValue, int count);

    void writeInfoLog(const QString& msg);
    void writeWarningLog(const QString& msg);
    void writeErrorLog(const QString& msg);

    void setActivityWidget(const bool activity);

    void closeEvent(QCloseEvent* event) override;

private slots:
    void on_actionAbout_triggered();


    void autoConnectionDBModule();
    void testBlockWritePerformanceModule();  // 测试分块写入性能

    void runTestModule();
    void selectSourceFile();
    void selectBlockFile();

private:
    Ui::MainWindow* ui;

    QString _host;
    qint16  _port;
    QString _driver;
    QString _user;
    QString _password;

    QSqlDatabase _curDB; // 当前操作的数据库对象
    QString _curDBName;  // 当前使用的数据库名字
    bool _isFinalConnDB; // 最终成功连接到数据库

};
#endif // MAINWINDOW_H
