#include "AsyncComputeModule.h"
#include "ThemeStyle.h"

#include <QDebug>
#include <QThread>

/**
 * 注意：这个类中所有的方法都是准备放置在子线程中执行的，内部包含了耗时的复杂计算任务
 */

AsyncComputeModule::AsyncComputeModule(QObject *parent)
    : QObject{parent}
{
    _dbs = new DatabaseService(this);

    emit signalWriteSuccLog(QString("Thread %1 init successed").arg(getCurrentThreadID()));
}

AsyncComputeModule::~AsyncComputeModule()
{
    if (_dbs->isDatabaseOpen())
    {
        _dbs->disconnectCurDatabase();
    }
    delete _dbs;

    emit signalFinished();
}

/**
 * @brief AsyncComputeModule::connectDatabase 连接到指定数据库（由于数据库操作不能跨线程，所以线程中要单独链接到数据库）
 * @param host 主机名或链接
 * @param port 端口号
 * @param driver Qt 数据库驱动
 * @param user 用户名
 * @param pwd 密码
 * @param database 数据库名称
 * @return 是否成功连接
 */
bool AsyncComputeModule::connectDatabase(const QString &host, const int port, const QString &driver, const QString &user, const QString &pwd, const QString &database)
{
    emit signalWriteInfoLog(QString("Thread %1: Start connect to %2:%3 database %4").arg(getCurrentThreadID(), host, QString::number(port), database));

    bool is_succ = _dbs->connectDatabase(host, port, driver, user, pwd, database);
    _last_log = QString("Thread %1: %2").arg(getCurrentThreadID(), _dbs->lastLog());
    if (!is_succ)
    {
        emit signalDbConnState(_dbs->isDatabaseOpen());
        emit signalWriteErrorLog(_last_log);
        emit signalErrorBox(_last_log);
        return false;
    }
    emit signalDbConnState(_dbs->isDatabaseOpen());
    emit signalWriteSuccLog(_last_log);
    emit signalInfoBox(_last_log);
    return true;
}

/**
 * @brief AsyncComputeModule::disconnectCurrentDatabase 断开当前线程与当前数据库的连接
 */
void AsyncComputeModule::disconnectCurrentDatabase()
{
    bool is_succ = _dbs->disconnectCurDatabase();
    _last_log = QString("Thread %1: %2").arg(getCurrentThreadID(), _dbs->lastLog());
#if !QT_NO_DEBUG
    qDebug() << _last_log;
#endif
    if (!is_succ)
    {
        emit signalWriteErrorLog(_last_log);
        return;
    }
    emit signalWriteSuccLog(_last_log);
    emit signalDbConnState(false);
}

void AsyncComputeModule::dropCurrentDatabase()
{
    bool is_succ = _dbs->dropCurDatabase();
    _last_log = QString("Thread %1: %2").arg(getCurrentThreadID(), _dbs->lastLog());
#if !QT_NO_DEBUG
    qDebug() << _last_log;
#endif
    if (!is_succ)
    {
        emit signalWriteErrorLog(_last_log);
        return;
    }
    emit signalWriteSuccLog(_last_log);
}

/**
 * @brief AsyncComputeModule::finishJob 结束计算任务（将会结束改线程）
 * @param drop_db 是否删除数据库
 */
void AsyncComputeModule::finishJob(const bool drop_db)
{
    if (drop_db)
    {
        dropCurrentDatabase();
    }
    disconnectCurrentDatabase();
    emit signalFinished();

    _last_log = "Send signal AsyncComputeModule::signalFinished()";
#if !QT_NO_DEBUG
    qDebug() << _last_log;
#endif
    return;
}

QString AsyncComputeModule::getCurrentThreadID() const
{
    return QString::number(reinterpret_cast<quintptr>(QThread::currentThreadId()), 16);
}

