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

    bool connectDatabase(const QString& host, const int port, const QString& driver, const QString& user, const QString& pwd);

    bool isDatabaseExist(const QString& db);
    bool createDatabase(const QString& db);
    bool useDatabase(const QString& db);

    void writeInfoLog(const QString& msg);
    void writeWarningLog(const QString& msg);
    void writeErrorLog(const QString& msg);


    void closeEvent(QCloseEvent* event) override;

private slots:
    void on_actionAbout_triggered();

private:
    Ui::MainWindow* ui;
    QSqlDatabase db;
    QString curUsingDB;  // 当前使用的数据库名字

};
#endif // MAINWINDOW_H
