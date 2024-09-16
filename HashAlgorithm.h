#ifndef HASHALGORITHM_H
#define HASHALGORITHM_H


#include <QString>
#include <QCryptographicHash>
#include <QByteArray>

enum HashAlg {
    MD5    = 0,
    SHA1   = 1,
    SHA256 = 2,
    SHA512 = 3
};


QByteArray getDataHash(const QByteArray& data, HashAlg alg)
{
    switch (alg) {
    case HashAlg::MD5:
        return QCryptographicHash::hash(data, QCryptographicHash::Md5);

    case HashAlg::SHA1:
        return QCryptographicHash::hash(data, QCryptographicHash::Sha1);

    case HashAlg::SHA256:
        return QCryptographicHash::hash(data, QCryptographicHash::Sha256);

    case HashAlg::SHA512:
        return QCryptographicHash::hash(data, QCryptographicHash::Sha512);

    default:
        break;
    }
}

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
