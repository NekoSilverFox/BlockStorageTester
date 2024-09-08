#ifndef HASHALGORITHM_H
#define HASHALGORITHM_H


#include <QString>
#include <QCryptographicHash>
#include <QByteArray>

namespace Hash
{

QString MD5(const QByteArray& data)
{
    QCryptographicHash hash(QCryptographicHash::Md5);
    hash.addData(data);
    return hash.result().toHex();   // 获取哈希结果并转换为十六进制字符串
}

QString SHA1(const QByteArray& data)
{
    QCryptographicHash hash(QCryptographicHash::Sha1);
    hash.addData(data);
    return hash.result().toHex();   // 获取哈希结果并转换为十六进制字符串
}

QString SHA256(const QByteArray& data)
{
    QCryptographicHash hash(QCryptographicHash::Sha256);
    hash.addData(data);
    return hash.result().toHex();   // 获取哈希结果并转换为十六进制字符串
}

QString SHA512(const QByteArray& data)
{
    QCryptographicHash hash(QCryptographicHash::Sha512);
    hash.addData(data);
    return hash.result().toHex();   // 获取哈希结果并转换为十六进制字符串
}

}

#endif // HASHALGORITHM_H
