#include "AsyncComputeModule.h"

#include <QDebug>
#include <QThread>
#include <QTimer>
#include <QElapsedTimer>

#include "InputFile.h"
#include "ThemeStyle.h"

/**
 * 注意：这个类中所有的方法都是准备放置在子线程中执行的，内部包含了耗时的复杂计算任务
 */
AsyncComputeModule::AsyncComputeModule(QObject *parent)
    : QObject{parent}
{
    _dbs = new DatabaseService(this);

    emit signalWriteSuccLog(QString("[Thread %1] init successed").arg(getCurrentThreadID()));
}

AsyncComputeModule::~AsyncComputeModule()
{
    if (_dbs->isDatabaseOpen())
    {
        _dbs->disconnectCurDatabase();
    }
    delete _dbs;

    emit signalAllJobFinished();
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
    emit signalWriteInfoLog(QString("[Thread %1] Start connect to %2:%3 database %4").arg(getCurrentThreadID(), host, QString::number(port), database));

    bool is_succ = _dbs->connectDatabase(host, port, driver, user, pwd, database);
    _last_log = QString("[Thread %1] %2").arg(getCurrentThreadID(), _dbs->lastLog());
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
    _last_log = QString("[Thread %1] %2").arg(getCurrentThreadID(), _dbs->lastLog());
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
    _last_log = QString("[Thread %1] %2").arg(getCurrentThreadID(), _dbs->lastLog());
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
 * @brief AsyncComputeModule::finishAllJob 结束计算任务（将会结束该线程）
 * @param drop_db 是否删除数据库
 */
void AsyncComputeModule::finishAllJob(const bool drop_db)
{
    if (drop_db)
    {
        dropCurrentDatabase();
        qDebug() << _last_log;
    }
    disconnectCurrentDatabase();
    emit signalAllJobFinished();

    _last_log = "Send signal AsyncComputeModule::signalAllJobFinished()";
#if !QT_NO_DEBUG
    qDebug() << _last_log;
#endif
    return;
}

/**
 * @brief AsyncComputeModule::runTestSegmentationProfmance 将源文件分块，计算哈希，然后写入数据库和文件
 * @param source_file_path 源文件路径
 * @param block_file_path 分块后记录源文件块哈希的文件
 * @param alg 哈希算法
 * @param block_size 块大小（Byte）
 */
void AsyncComputeModule::runTestSegmentationProfmance(const QString& source_file_path, const QString& block_file_path,
                                                const HashAlg alg, const size_t block_size)
{
    emit signalWriteInfoLog(QString("[Thread %1] Start test block segmentation performance").arg(getCurrentThreadID()));

    /* 为了避免意外操作，暂时禁用按钮 */
    emit signalSetActivityWidget(false);

    /* 数据库无连接 */
    if (!_dbs->isDatabaseOpen())
    {
        _last_log = QString("[Thread %1] Database do not connected, test exit").arg(getCurrentThreadID());
        emit signalWriteErrorLog(_last_log);
        emit signalWarnBox(_last_log);

        emit signalSetActivityWidget(true);
        emit signalTestSegmentationPerformanceFinished(false);
        return;
    }

    /* 打开源文件（输入） */
    InputFile* fin = new InputFile(this, source_file_path);
    bool is_succ = fin->isOpen();
    _last_log = QString("[Thread %1] %2").arg(getCurrentThreadID(), fin->lastLog());
    if (!is_succ)
    {
        emit signalWriteErrorLog(_last_log);
        emit signalErrorBox(_last_log);

        emit signalSetActivityWidget(true);
        emit signalTestSegmentationPerformanceFinished(false);
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

        _last_log = QString("[Thread %1] Successed create Hash-Block %2").arg(getCurrentThreadID(), blockInfo.filePath());
        emit signalWriteSuccLog(_last_log);
    }
    else
    {
        _last_log = QString("[Thread %1] Can not create Hash-Block %2").arg(getCurrentThreadID(), blockInfo.filePath());
        emit signalWriteErrorLog(_last_log);
        emit signalErrorBox(_last_log);

        emit signalSetActivityWidget(true);
        emit signalTestSegmentationPerformanceFinished(false);
        return;
    }


    /* 创建表 */
    QString tb = getTableName(block_size, alg);  // 根据算法和块大小自动创建表名
    bool is_exists = _dbs->isTableExists(tb);
    if (is_exists)
    {
        emit signalWriteWarningLog(QString("[Thread %1] %2").arg(getCurrentThreadID(), _dbs->lastLog()));
    }
    else
    {
        emit signalWriteInfoLog(QString("[Thread %1] Create table `%2`").arg(getCurrentThreadID(), tb));
        is_succ = _dbs->createBlockInfoTable(tb);
        emit signalWriteInfoLog(QString("[Thread %1] Run SQL `%2`").arg(getCurrentThreadID(), _dbs->lastSQL()));
        _last_log = QString("[Thread %1] %2").arg(getCurrentThreadID(), _dbs->lastLog());
        if (!is_succ)
        {
            emit signalWriteErrorLog(_last_log);
            emit signalErrorBox(_last_log);

            emit signalSetActivityWidget(true);
            emit signalTestSegmentationPerformanceFinished(false);
            return;
        }
        emit signalWriteSuccLog(_last_log);
    }

    /* 更新（初始化） UI 信息 */
    emit signalWriteInfoLog(QString("[Thread %1] "
                                    "From Source-File: %2, Size: %3<br>"
                                    "to Hash-Block File: %4 and Table: %5<br>"
                                    "with Hash-Alg: %6, Segmentation-Block-Size: %7 Bytes").arg(getCurrentThreadID(), fin->filePath(), QString::number(fin->fileSize()), blockInfo.filePath(), tb, Hash::getHashName(alg), QString::number(block_size)));
    emit signalSetLbRuningJobInfo(QString("Job: Test segmentation profmance | Hash alg: %1 | Block size: %2 | DB-Table: %3").arg(Hash::getHashName(alg), QString::number(block_size), tb));
    emit signalSetProgressBarRange(0, fin->fileSize());
    emit signalSetProgressBarValue(0);
    emit signalSetLbSegmentationStyle(ThemeStyle::LABLE_ORANGE);
    emit signalSetLbRecoverStyle(ThemeStyle::LABLE_RED);

    /**
     * 开始读取 -> 计算哈希 -> 写入
     */
    QByteArray buf_block;       // 读取的源文件的 buffer（用来暂存当前块）
    qint64 ptr_loc = 0;         // 读取指针目前所处的位置
    size_t cur_block_size = 0;  // 本次读取块的大小（因为文件末尾最后块的大小有可能不是块大小的整数倍）
    QByteArray buf_hash;        // 存储当前块的哈希值
    size_t repeat_times = 0;    // 当前块的重复次数
    size_t total_repeat_times = 0;  // 所有哈希值/块的重复次数
    size_t total_hash_records = 0; // 当前数据表中的哈希记录条数
    const size_t file_blocks = (fin->fileSize() + block_size - 1) / block_size; // 文件一共会被分多少块由于可能除不尽，这里使用简单的向上取整算法
    emit signalSetLcdTotalFileBlocks(file_blocks);


    /* 计算耗时 */
    QElapsedTimer elapsed_time;
    elapsed_time.start();

    while (!fin->atEnd())
    {
        buf_block = fin->read(block_size);
        cur_block_size = buf_block.size();       // 计算当前读取的字节数，防止越界
        buf_hash = Hash::getDataHash(buf_block, alg);  // 计算哈希（非性能瓶颈）

        /* 写入数据库 */
        repeat_times = _dbs->getHashRepeatTimes(tb, buf_hash);

#if !QT_NO_DEBUG
        qDebug() << _dbs->lastLog();
#endif

        if (0 == repeat_times)  // 重复次数为 0 说明没有记录过当前哈希
        {
            ++total_hash_records;
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

        // out << buf_hash;           // 记录哈希到文件【注意】通过 `<<` QDataStream 写入时，QDataStream 会在字符串的前面写入一个4字节的长度字段，表示接下来数据的长度
        out.writeRawData(buf_hash, buf_hash.size());
        ptr_loc += cur_block_size; // 移动指针位置

        /* 刷新 ui */
        if (0 == ptr_loc % 16 || fin->atEnd())
        {
            emit signalSetProgressBarValue(ptr_loc);
            emit signalSetLcdTotalDbHashRecords(total_hash_records);
            emit signalSetLcdTotalRepeat(total_repeat_times);
            emit signalSetLcdRepeatPercent((double)total_repeat_times/file_blocks*100);
            emit signalSetLcdSegmentationTime((double)(elapsed_time.elapsed() / 1000.0));
        }
    }
    _cur_result_comput                = ResultComput();
    _cur_result_comput.sourceFilePath = fin->filePath();
    _cur_result_comput.hashAlg        = alg;
    _cur_result_comput.blockSize      = block_size;
    _cur_result_comput.totalBlock     = file_blocks;
    _cur_result_comput.hashRecordDB   = total_hash_records;
    _cur_result_comput.repeatRecord   = total_repeat_times;
    _cur_result_comput.repeatRate     = (double)total_repeat_times/file_blocks*100;
    _cur_result_comput.segTime        = (double)(elapsed_time.elapsed() / 1000.0);

    blockFile.close();
    delete fin;

    emit signalCurSegmentationResult(_cur_result_comput);

    _last_log = QString("[Thread %1] Finish test writing performance, use time %2 sec").arg(getCurrentThreadID(), QString::number(_cur_result_comput.segTime));
    emit signalWriteSuccLog(_last_log);

    /* 解锁按钮 */
    emit signalSetActivityWidget(true);
    emit signalSetLbSegmentationStyle(ThemeStyle::LABLE_GREEN);

    /* 发送结束信号 */
    emit signalTestSegmentationPerformanceFinished(true);
}

/**
 * @brief AsyncComputeModule::runTestRecoverProfmance 子线程开始恢复文件
 * @param recover_file_path  要恢复至的文件的文件名
 * @param block_file_path 存储哈希块的文件
 * @param alg 哈希算法
 * @param block_size 块大小
 */
void AsyncComputeModule::runTestRecoverProfmance(const QString &recover_file_path, const QString& block_file_path, const HashAlg alg, const size_t block_size)
{
    emit signalWriteInfoLog(QString("[Thread %1] Start test block recover performance").arg(getCurrentThreadID()));

    /* 为了避免意外操作，暂时禁用按钮 */
    emit signalSetActivityWidget(false);

    /* 数据库无连接 */
    if (!_dbs->isDatabaseOpen())
    {
        _last_log = QString("[Thread %1] Database do not connected, test exit").arg(getCurrentThreadID());
        emit signalWriteErrorLog(_last_log);
        emit signalWarnBox(_last_log);

        emit signalSetActivityWidget(true);
        emit signalTestRecoverPerformanceFinished(false);
        return;
    }

    /* 打开块文件（输入） */
    InputFile* fin = new InputFile(this, block_file_path);
    bool is_succ = fin->isOpen();
    _last_log = QString("[Thread %1] %2").arg(getCurrentThreadID(), fin->lastLog());
    if (!is_succ)
    {
        emit signalWriteErrorLog(_last_log);
        emit signalErrorBox(_last_log);

        emit signalSetActivityWidget(true);
        emit signalTestRecoverPerformanceFinished(false);
        return;
    }
    emit signalWriteSuccLog(_last_log);

    /* 创建恢复的文件（输出） */
    QFile recoverFile(recover_file_path);
    QFileInfo recoverFileInfo;
    QDataStream out;
    if (recoverFile.open(QIODevice::WriteOnly))
    {
        out.setDevice(&recoverFile);
        out.setVersion(QDataStream::Qt_DefaultCompiledVersion);  // 设置流的版本（可以根据实际情况设置，通常用于处理跨版本兼容性）
        recoverFileInfo.setFile(recoverFile);

        _last_log = QString("[Thread %1] Successed create Recover-Block %2").arg(getCurrentThreadID(), recoverFileInfo.filePath());
        emit signalWriteSuccLog(_last_log);
    }
    else
    {
        _last_log = QString("[Thread %1] Can not create Recover-Block %2").arg(getCurrentThreadID(), recoverFileInfo.filePath());
        emit signalWriteErrorLog(_last_log);
        emit signalErrorBox(_last_log);

        emit signalSetActivityWidget(true);
        emit signalTestRecoverPerformanceFinished(false);
        return;
    }

    /* 根据哈希值和块大小判断要读取的表是否存在 */
    QString tb = getTableName(block_size, alg);
    if (!_dbs->isTableExists(tb))
    {
        _last_log = QString("[Thread %1] Exit Test Recover Profmance: %2").arg(getCurrentThreadID(), _dbs->lastLog());
        emit signalWriteErrorLog(_last_log);

        emit signalSetActivityWidget(true);
        emit signalTestRecoverPerformanceFinished(false);
        return;
    }
    _last_log = QString("[Thread %1] %2").arg(getCurrentThreadID(), _dbs->lastLog());
    emit signalWriteSuccLog(_last_log);


    /* 计算耗时，这里 QTime 不起作用，因为开始计算后线程一只处于阻塞状态 */
    QElapsedTimer elapsed_time;
    elapsed_time.start();

    /* 其他用于恢复原信息需要用到的对象 */
    InputFile* curSourceFile = nullptr;            // 用于读取源文件
    BlockInfo cur_block_info;
    const size_t hash_size = Hash::getHashSize(alg);  // 获取哈希块文件中，每个哈希的长度（这个长度是固定的）
    const size_t num_need_recover = fin->fileSize() / hash_size;  // .bkh 文件中记录的哈希记录条数，同样的也是需要从源文件恢复几个块
    QByteArray buf_hash;                        // 用于读取块文件中存储的哈希值，读取的长度为 hash_size
    size_t total_cant_revcover = 0;             // 无法恢复块的数量（数据库中没记录这个块）
    const QByteArray blank_block(block_size, '\0'); // 如果没找到这个哈希值的源数据块，用这个全是 0 的数据填充 '\0' 是 ASCII 表中的空字符，对应二进制 0

    /* 更新（初始化） UI 信息 */
    emit signalWriteInfoLog(QString("[Thread %1] "
                                    "From Hash-Block-File: %2, Size: %3<br>"
                                    "To Recover-File: %4<br>"
                                    "With:<br>"
                                    "Hash alg: %5, Hash-Length: %6<br>"
                                    "Every recover block size: %7<br>"
                                    "DB-Table: %8").arg(getCurrentThreadID(), fin->filePath(), QString::number(fin->fileSize()),
                                                        recoverFileInfo.filePath(), Hash::getHashName(alg), QString::number(hash_size), QString::number(block_size), tb));
    emit signalSetLbRuningJobInfo(QString("Job: Test recover profmance | Hash alg: %1 | Block size: %2 | DB-Table: %3").arg(Hash::getHashName(alg), QString::number(block_size), tb));
    emit signalSetLcdNumNeedRecover(num_need_recover);
    emit signalSetProgressBarRange(0, fin->fileSize());  // 以读取块文件的指针位置作为进度
    emit signalSetProgressBarValue(0);
    emit signalSetLbRecoverStyle(ThemeStyle::LABLE_ORANGE);

    while (!fin->atEnd())
    {
        buf_hash = fin->read(hash_size);
        cur_block_info = _dbs->getBlockInfo(tb, buf_hash);
#if !QT_NO_DEBUG
        qDebug() << "\n[AsyncComputeModule::runTestRecoverProfmance] Read Hash: " << buf_hash.toHex() << "\nIn source: " << cur_block_info.filePath << "\nLoction: " << cur_block_info.location << " Size: " << cur_block_info.size;
#endif
        /* 更新 ui */
        emit signalSetProgressBarValue(fin->curPtrPostion());

        /* 数据库中没有记录当前块 */
        if (0 == cur_block_info.size)
        {
            _last_log = QString("[Thread %1] %2").arg(getCurrentThreadID(), _dbs->lastLog());
            ++total_cant_revcover;
            emit signalSetLcdTotalUnrecovered(total_cant_revcover);
            // out << blank_block;
            out.writeRawData(blank_block, block_size);

            emit signalWriteWarningLog(_last_log);
#if !QT_NO_DEBUG
            qDebug() << _last_log;
#endif
            continue;
        }

        /* 如果数据库中记录了当前块
         * 更新要读取源数据的源文件
         *  （这里可以优化，比如将数据表中的数据按照文件路径排序） */
        if (nullptr == curSourceFile || curSourceFile->filePath() != cur_block_info.filePath)
        {
            delete curSourceFile;
            curSourceFile = new InputFile(nullptr, cur_block_info.filePath);
            _last_log = QString("[Thread %1] Try to open source file %2").arg(getCurrentThreadID(), cur_block_info.filePath);
        }

        /* 可能出现源文件丢失的情况 */
        if (!curSourceFile->isOpen())
        {
            ++total_cant_revcover;
            emit signalSetLcdTotalUnrecovered(total_cant_revcover);
            out.writeRawData(blank_block, block_size);

            _last_log = QString("[Thread %1] %2").arg(getCurrentThreadID(), curSourceFile->lastLog());
            emit signalWriteWarningLog(_last_log);
            continue;
        }

        // out << fin->readFrom(cur_block_info.location, cur_block_info.size);
        out.writeRawData(curSourceFile->readFrom(cur_block_info.location, cur_block_info.size), cur_block_info.size);

        ++_cur_result_comput.recoveredBlock;  // 成功恢复块的数量（数据库中记录了这个块，并且源文件也成功读取了）
        _cur_result_comput.recoveredRate = (double)_cur_result_comput.recoveredBlock / num_need_recover * 100;  // 恢复成功率
        _cur_result_comput.recoveredTime = elapsed_time.elapsed() / 1000.0;  // 用时

        emit signalSetLcdTotalRecovered(_cur_result_comput.recoveredBlock);
        emit signalSetLcdRecoveredPercent(_cur_result_comput.recoveredRate);
        emit signalSetLcdRecoverTime(_cur_result_comput.recoveredTime);
#if !QT_NO_DEBUG
        qDebug() << _last_log << "\nRecover data: " << curSourceFile->readFrom(cur_block_info.location, cur_block_info.size);
#endif
    }

    // 结果发送到表
    emit signalCurRecoverResult(_cur_result_comput);
    recoverFile.close();
    delete curSourceFile;
    delete fin;

    _last_log = QString("[Thread %1] Successful recovery file to %2<br>"
                        "Number of unrecoverable blocks %3/%4, Recovery rate %5\%,"
                        "Use time: %6 sec").arg(getCurrentThreadID(), recover_file_path,
                                                QString::number(total_cant_revcover),
                                                QString::number(num_need_recover), QString::number(_cur_result_comput.recoveredRate, 'f', 2),
                                                QString::number(_cur_result_comput.recoveredTime));
    emit signalWriteSuccLog(_last_log);

    /* 解锁按钮 */
    emit signalSetActivityWidget(true);
    emit signalSetLbRecoverStyle(ThemeStyle::LABLE_GREEN);
    emit signalTestRecoverPerformanceFinished(true);
}

/**
 * @brief AsyncComputeModule::runTestSingal 执行单步测试（分块性能 + 恢复性能）
 * @param source_file_path 源文件路径
 * @param block_file_path 存储哈希块的文件
 * @param recover_file_path 要恢复至的文件的文件名
 * @param alg 哈希算法
 * @param block_size 块大小
 */
void AsyncComputeModule::runSingleTest(const QString &source_file_path, const QString &block_file_path,
                                       const QString &recover_file_path, const HashAlg alg, const size_t block_size)
{
    emit signalWriteInfoLog(QString("[Thread %1] Start to run Singal Test").arg(getCurrentThreadID()));

    /* 数据库无连接 */
    if (!_dbs->isDatabaseOpen())
    {
        _last_log = QString("[Thread %1] Database do not connected, test exit").arg(getCurrentThreadID());
        emit signalWriteErrorLog(_last_log);
        emit signalWarnBox(_last_log);

        emit signalSetActivityWidget(true);
        emit signalTestRecoverPerformanceFinished(false);
        return;
    }

    runTestSegmentationProfmance(source_file_path, block_file_path, alg, block_size);
    runTestRecoverProfmance(recover_file_path, block_file_path, alg, block_size);

    emit signalWriteSuccLog(QString("[Thread %1] Singal Test done").arg(getCurrentThreadID()));
}


/**
 * @brief AsyncComputeModule::runBenchmarkTest 自动执行基准测试
 * @param source_file_path 源文件路径
 * @param block_file_path 存储哈希块的文件
 * @param recover_file_path 要恢复至的文件的文件名
 * @param alg 哈希算法
 * @param block_size_list
 */
void AsyncComputeModule::runBenchmarkTest(const QString &source_file_path, const QString &block_file_path, const QString &recover_file_path,
                                          const HashAlg alg, const QList<size_t> &block_size_list)
{
    emit signalWriteInfoLog(QString("[Thread %1] Start to run Benchmark Test").arg(getCurrentThreadID()));

    /* 为了避免意外操作，暂时禁用按钮 */
    emit signalSetActivityWidget(false);

    /* 数据库无连接 */
    if (!_dbs->isDatabaseOpen())
    {
        _last_log = QString("[Thread %1] Database do not connected, test exit").arg(getCurrentThreadID());
        emit signalWriteErrorLog(_last_log);
        emit signalWarnBox(_last_log);

        emit signalSetActivityWidget(true);
        emit signalTestRecoverPerformanceFinished(false);
        return;
    }

    QString tb;
    for (size_t block_size : block_size_list)
    {
        /* 保证测试准确，每次都先删除指定表  */
        tb = getTableName(block_size, alg);
        if (_dbs->isTableExists(tb))
        {

            _dbs->deleteTable(tb);
        }

        emit signalWriteInfoLog(QString("[Thread %1] Benchmark Test with Block Size %2 Bytes, Hash-Alg %3").arg(getCurrentThreadID(), QString::number(block_size), Hash::getHashName(alg)));

        runTestSegmentationProfmance(source_file_path, block_file_path, alg, block_size);
        emit signalAddPointSegTimeAndRepeateRate(_cur_result_comput);

        runTestRecoverProfmance(recover_file_path, block_file_path, alg, block_size);
        emit signalAddPointRecoverTime(_cur_result_comput);
    }

    emit signalWriteSuccLog(QString("[Thread %1] Benchmark Test done").arg(getCurrentThreadID()));
}

QString AsyncComputeModule::getCurrentThreadID() const
{
    return QString::number(reinterpret_cast<quintptr>(QThread::currentThreadId()), 16);
}

/**
 * @brief AsyncComputeModule::getTableName 根据算法和块大小构建表名
 * @param block_size 块大小
 * @param alg 哈希算法
 * @return
 */
QString AsyncComputeModule::getTableName(const size_t block_size, const HashAlg alg)
{
    return QString("tb_%1bytes_%2").arg(QString::number(block_size), Hash::getHashName(alg)).toLower();
}

