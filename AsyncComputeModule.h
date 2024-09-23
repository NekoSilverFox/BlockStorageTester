#ifndef ASYNCCOMPUTEMODULE_H
#define ASYNCCOMPUTEMODULE_H

#include <QObject>

#include "DatabaseService.h"
#include "HashAlgorithm.h"

/**
 * @brief [Asynchronous Computation Module] 异步计算模块，执行所需的数据库、文件IO、计算等复杂的或耗时的计算任务。注意：最好使用单独的线程调用这个模块
 */
class AsyncComputeModule : public QObject
{
    Q_OBJECT
public:
    explicit AsyncComputeModule(QObject *parent = nullptr);
    ~AsyncComputeModule();

    /* 数据库操作 */
    bool connectDatabase(const QString& host, const int port, const QString& driver,
                         const QString& user, const QString& pwd, const QString& database);
    void disconnectCurrentDatabase();
    void dropCurrentDatabase();
    void finishAllJob(const bool drop_db);  // 同时也是最后的数据库断开连接和删除操作，发出最终的退出信号

    /* 计算任务 */
    void runTestSegmentationProfmance(const QString& source_file_path, const QString& block_file_path, const HashAlg alg, const size_t block_size);
    void runTestRecoverProfmance(const QString& recover_file_path, const QString& block_file_path, const HashAlg alg, const size_t block_size);


signals:
    /* 设置UI */
    void signalSetLbDBConnectedStyle(QString style);
    void signalSetLbSegmentationStyle(QString style);
    void signalSetLbRecoverStyle(QString style);

    void signalSetActivityWidget(const bool activity);

    void signalSetLbRuningJobInfo(const QString& info);

    void signalSetProgressBarValue(const int number);
    void signalSetProgressBarRange(const int minimum, const int maximum);

    void signalSetLcdTotalFileBlocks(const int number);
    void signalSetLcdTotalDbHashRecords(const int number);
    void signalSetLcdTotalRepeat(const int number);
    void signalSetLcdRepeatPercent(const double number);  // 注意：这个是 Repeat（重复）
    void signalSetLcdSegmentationTime(const double number);

    void signalSetLcdNumNeedRecover(const int number);
    void signalSetLcdTotalUnrecovered(const int number);
    void signalSetLcdTotalRecovered(const int number);
    void signalSetLcdRecoveredPercent(const double number);  // 注意：这个是 Recovered（恢复）
    void signalSetLcdRecoverTime(const double number);


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
    void signalAllJobFinished();
    void signalFinishAllJob(const bool drop_db);  // 任务完成信号，用于通知线程退出
    void signalConnDb(const QString& host, const int port, const QString& driver,
                      const QString& user, const QString& pwd, const QString& database);  // 连接数据库
    void signalDisconnDb();
    void signalDbConnState(const bool is_conn);  // 当前连接状态
    void signalDropCurDb();

    /* 计算任务信号 */
    void signalRunTestSegmentationPerformance(const QString& source_file_path, const QString& block_file_path,
                                              const HashAlg alg, const size_t block_size);
    bool signalTestSegmentationPerformanceFinished(const bool is_succ);  // ↑ 任务完成信号

    void signalRunTestRecoverProfmance(const QString& recover_file_path, const QString& block_file_path, const HashAlg alg, const size_t block_size);
    bool signalTestRecoverPerformanceFinished(const bool is_succ);        // ↑ 任务完成信号

private:
    QString getCurrentThreadID() const;
    QString getTableName(const size_t block_size, const HashAlg alg);

private:
    DatabaseService* _dbs; // 当前操作的数据库对象
    QString _last_log;     // 最后一条日志信息
};

#endif // ASYNCCOMPUTEMODULE_H
