#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

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

    void closeEvent(QCloseEvent* event) override;

private slots:
    void on_actionAbout_triggered();

private:
    Ui::MainWindow *ui;

};
#endif // MAINWINDOW_H
