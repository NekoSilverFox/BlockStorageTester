#include "InputFile.h"

InputFile::InputFile(QObject *parent, const QString& filePath)
    : QObject{parent}
{
    if (filePath.isEmpty())
    {
        qFatal("File path is empty");
        return;
    }
    setFile(filePath);
}

bool InputFile::setFile(const QString& filePath)
{
    if (_file.isOpen())
    {
        _file.close();
    }

    _file.setFileName(filePath);
    if (!_file.open(QIODevice::ReadOnly))
    {
        _last_log = QString("Can not open file %1").arg(_info.filePath());
        return false;
    }

    _sin.setDevice(&_file);
    // 设置流的版本（可以根据实际情况设置，通常用于处理跨版本兼容性）
    _sin.setVersion(QDataStream::Qt_DefaultCompiledVersion);
    _info.setFile(_file);
    _last_log = QString("Successed open file %1, size %2 Bytes").arg(_info.filePath(), QString::number(_info.size()));
    return true;
}

InputFile::~InputFile()
{
    if (_file.isOpen())
    {
        _file.close();
    }
}

bool InputFile::isOpen()
{
    return _file.isOpen();
}

/**
 * @brief InputFile::sin 获取文件输入流
 * @return 文件输入流
 */
QDataStream& InputFile::sin()
{
    return _sin;
}

QByteArray InputFile::read(const qint64 maxlen)
{
    return _file.read(maxlen);
}

/**
 * @brief InputFile::readFrom 从指定位置读取指定数量的字节
 * @param position 位置
 * @param maxlen 读取的字节
 * @return
 */
QByteArray InputFile::readFrom(const qint64 location, const qint64 maxlen)
{
    // 移动文件指针到指定的位置
    if (!_file.seek(location))
    {
        _last_log = "Unable to move file pointer to specified location";
        return nullptr;
    }

    return _file.read(maxlen);
}

/**
 * @brief InputFile::curPtrPostion 当前输入流指针位于文件的位置
 * @return 指针位置
 */
qint64 InputFile::curPtrPostion()
{
    return _file.pos();
}

bool InputFile::atEnd()
{
    return _file.atEnd();
}

/**
 * @brief InputFile::fileSize 获取文件大小（Byte）
 * @return 文件大小（Byte）
 */
qint64 InputFile::fileSize()
{
    return _info.size();
}


/**
 * @brief InputFile::filePath 获取文件路径
 * @return 文件路径
 */
QString InputFile::filePath()
{
    return _info.filePath();
}


QString InputFile::lastLog()
{
    return _last_log;
}
