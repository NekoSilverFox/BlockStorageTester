#include "AsyncComputeModule.h"

#include <QDebug>
#include <QThread>
#include <QTimer>
#include <QElapsedTimer>

#include "InputFile.h"

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

/**
 * @brief AsyncComputeModule::runBlockWriteProfmance 将源文件分块，计算哈希，然后写入数据库和文件
 * @param source_file_path 源文件路径
 * @param block_file_path 分块后记录源文件块哈希的文件
 * @param alg 哈希算法
 * @param block_size 块大小（Byte）
 */
void AsyncComputeModule::runBlockWriteProfmance(const QString& source_file_path, const QString& block_file_path,
                                                const HashAlg alg, const size_t block_size)
{
    emit signalWriteInfoLog(QString("Thread %1: Start test block write performance: Source file: %2; Hash-Block File: %3, Hash-Alg: %4, Block Size: %5 Bytes").arg(getCurrentThreadID(), source_file_path, block_file_path,  QString::number(alg), QString::number(block_size)));

    /* 为了避免意外操作，暂时禁用按钮 */
    emit signalSetActivityWidget(false);

    /* 数据库无连接 */
    if (!_dbs->isDatabaseOpen())
    {
        _last_log = QString("Thread %1: Database do not connected, test exit").arg(getCurrentThreadID());
        emit signalWriteErrorLog(_last_log);
        emit signalWarnBox(_last_log);

        emit signalSetActivityWidget(true);
        return;
    }

    /* 打开源文件（输入） */
    InputFile* fin = new InputFile(this, source_file_path);
    bool is_succ = fin->isOpen();
    _last_log = QString("Thread %1: %2").arg(getCurrentThreadID(), fin->lastLog());
    if (!is_succ)
    {
        emit signalWriteErrorLog(_last_log);
        emit signalErrorBox(_last_log);

        emit signalSetActivityWidget(true);
        return;
    }
    emit signalWriteSuccLog(_last_log);

    /* 创建块文件（输出） */
    QFile blockFile(block_file_path);
    QFileInfo blockInfo;
    QDataStream out;
    if (blockFile.open(QIODevice::WriteOnly))
    {
        out.setDevice(&blockFile);
        out.setVersion(QDataStream::Qt_DefaultCompiledVersion);  // 设置流的版本（可以根据实际情况设置，通常用于处理跨版本兼容性）
        blockInfo.setFile(blockFile);

        _last_log = QString("Thread %1: Successed create Hash-Block %2").arg(getCurrentThreadID(), blockInfo.filePath());
        emit signalWriteSuccLog(_last_log);
    }
    else
    {
        _last_log = QString("Thread %1: Can not create Hash-Block %2").arg(getCurrentThreadID(), blockInfo.filePath());
        emit signalWriteErrorLog(_last_log);
        emit signalErrorBox(_last_log);

        emit signalSetActivityWidget(true);
        return;
    }


    /* 创建表 */
    QString tb = QString("tb_%1bytes_%2").arg(QString::number(block_size), Hash::getHashName(alg)).toLower();
    emit signalWriteInfoLog(QString("Thread %1: Create table `%2`").arg(getCurrentThreadID(), tb));
    is_succ = _dbs->createBlockInfoTable(tb);
    emit signalWriteInfoLog(QString("Thread %1: Run SQL `%2`").arg(getCurrentThreadID(), _dbs->lastSQL()));
    _last_log = QString("Thread %1: %2").arg(getCurrentThreadID(), _dbs->lastLog());
    if (!is_succ)
    {
        emit signalWriteErrorLog(_last_log);
        emit signalErrorBox(_last_log);

        emit signalSetActivityWidget(true);
        return;
    }
    emit signalWriteSuccLog(_last_log);

    /* 更新（初始化） UI 信息 */
    emit signalSetLbRuningJobInfo(QString("Job: Test write profmance | Hash alg: %1 | Block size: %2 | DB-Table: %3").arg(Hash::getHashName(alg), QString::number(block_size), tb));
    emit signalSetProgressBarRange(0, fin->fileSize());
    emit signalSetProgressBarValue(0);

    /**
     * 开始读取 -> 计算哈希 -> 写入
     */
    QByteArray buf_block;       // 读取的源文件的 buffer（用来暂存当前块）
    qint64 ptr_loc = 0;         // 读取指针目前所处的位置
    size_t cur_block_size = 0;  // 本次读取块的大小（因为文件末尾最后块的大小有可能不是块大小的整数倍）
    QByteArray buf_hash;        // 存储当前块的哈希值
    size_t repeat_times = 0;    // 当前块的重复次数
    size_t total_repeat_times = 0;
    size_t total_hash_blocks = 0;
    const size_t file_blocks = fin->fileSize() / block_size;// 文件一共会被分多少块
    emit signalSetLcdTotalFileBlocks(file_blocks);

    /* 计算耗时 */
    QElapsedTimer elapsed_time;
    elapsed_time.start();
    while (!fin->atEnd())
    {
        buf_block = fin->read(block_size);
        cur_block_size = buf_block.size();       // 计算当前读取的字节数，防止越界
        buf_hash = Hash::getDataHash(buf_block, alg);  // 计算哈希

        /* 写入数据库 */
        repeat_times = _dbs->getHashRepeatTimes(tb, buf_hash);

#if !QT_NO_DEBUG
        qDebug() << _dbs->lastLog();
#endif
        if (0 == repeat_times)  // 重复次数为 0 说明没有记录过当前哈希
        {
            ++total_hash_blocks;
            _dbs->insertNewBlockInfoRow(tb, buf_hash, fin->filePath(), ptr_loc, cur_block_size);
        }
        else
        {
            ++total_repeat_times;
            _dbs->updateCounter(tb, buf_hash, (repeat_times + 1));
        }

#if !QT_NO_DEBUG
        qDebug() << _dbs->lastLog();
        qDebug() << QString("↳ Read: %1, Hash: %2, Pointer location: %3, Repet times: %4").arg(
            buf_block.toHex(), buf_hash.toHex(), QString::number(ptr_loc), QString::number(repeat_times));  // 输出读取的内容（十六进制格式显示）
        qDebug() << "---------------------------------";
#endif
        out << buf_hash;           // 记录哈希到文件
        ptr_loc += cur_block_size; // 移动指针位置

        /* 刷新 ui */
        if (0 == ptr_loc % 16 || fin->atEnd())
        {
            emit signalSetProgressBarValue(ptr_loc);
            emit signalSetLcdTotalHashBlocks(total_hash_blocks);
            emit signalSetLcdTotalRepeatBlocks(QString("%1  Per:%2").arg(total_repeat_times, (double)total_repeat_times / file_blocks));
            emit signalSetLcdUseTime((double)(elapsed_time.elapsed() / 1000.0));
        }
    }
    blockFile.close();

    _last_log = QString("Thread %1: Finish test writing performance, use time %2 sec").arg(getCurrentThreadID(), (double)(elapsed_time.elapsed() / 1000.0));

    emit signalWriteSuccLog(_last_log);

    /* 解锁按钮 */
    emit signalSetActivityWidget(true);
}

QString AsyncComputeModule::getCurrentThreadID() const
{
    return QString::number(reinterpret_cast<quintptr>(QThread::currentThreadId()), 16);
}

