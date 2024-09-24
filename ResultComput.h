#ifndef RESULTCOMPUT_H
#define RESULTCOMPUT_H

#include <QString>
#include "HashAlgorithm.h"

/**
 * @brief 用于存储计算任务的结果
 */
struct ResultComput
{
    QString sourceFilePath  =   "";     // 源文件路径
    HashAlg hashAlg         =   HashAlg::NONE;  // 分块/恢复所用的哈希函数
    size_t  blockSize       =   0;      // 分块大小
    size_t  totalBlock      =   0;      // 源文件被分成了多少块
    size_t  hashRecordDB    =   0;      // 数据库数据表中中记录的哈希条数
    size_t  repeatRecord    =  0;       // 重复的哈希值/块数量
    double  repeatRate      =   0.0;    // 重复率
    double  segTime         =   0.0;    // 分块任务所用的时间
    size_t  recoveredBlock  =   0;      // 成功恢复的块数量
    double  recoveredRate   =   0.0;    // 恢复率
    double  recoveredTime   =   0.0;    // 恢复任务所用时间
};

#endif // RESULTCOMPUT_H
