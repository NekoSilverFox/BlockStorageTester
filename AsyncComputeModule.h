#ifndef ASYNCCOMPUTEMODULE_H
#define ASYNCCOMPUTEMODULE_H

#include "DatabaseService.h"

#include <QObject>

/**
 * @brief [Asynchronous Computation Module] 异步计算模块，执行所需的数据库、文件IO、计算等复杂的或耗时的计算任务。注意：最好使用单独的线程调用这个模块
 */
class AsyncComputeModule : public QObject
{
    Q_OBJECT
public:
    explicit AsyncComputeModule(QObject *parent = nullptr);
    ~AsyncComputeModule();


    bool connectDatabase(const QString& host, const int port, const QString& driver,
                         const QString& user, const QString& pwd, const QString& database);
    void disconnectCurrentDatabase();
    void dropCurrentDatabase();
    void finishJob(const bool drop_db);


signals:
    /* 设置指示灯 */
    void signalSetLbDBConnectedStyle(QString style);

    /* 日志信号 */
    void signalWriteInfoLog(const QString& msg);
    void signalWriteWarningLog(const QString& msg);
    void signalWriteErrorLog(const QString& msg);
    void signalWriteSuccLog(const QString& msg);

    /* 让主线程调用 QMessageBox 信号 */
    void signalInfoBox(const QString& msg);
    void signalWarnBox(const QString& msg);
    void signalErrorBox(const QString& msg);


    /* 数据库信号 */
    void signalFinished();
    void signalFinishJob(const bool drop_db);  // 任务完成信号，用于通知线程退出
    void signalConnDb(const QString& host, const int port, const QString& driver,
                      const QString& user, const QString& pwd, const QString& database);  // 连接数据库
    void signalDisconnDb();
    void signalDbConnState(const bool is_conn);  // 当前连接状态
    void signalDropCurDb();

private:
    QString getCurrentThreadID() const;

private:
    DatabaseService* _dbs; // 当前操作的数据库对象
    QString _last_log;     // 最后一条日志信息
};

#endif // ASYNCCOMPUTEMODULE_H
