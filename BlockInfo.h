#ifndef BLOCKINFO_H
#define BLOCKINFO_H

#include <QString>

/**
 * @brief 存储块的信息（用于定位块的位置）
 */
struct BlockInfo {
    QString filePath;   // 块所在的源文件位置
    qint64 location;    // 块在文件的（起始）位置（在第几Byte）
    size_t size;  // 块的大小（Byte）
};

#endif // BLOCKINFO_H
