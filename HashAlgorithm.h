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

/**
 * @brief getDataHash
 * @param data 数据
 * @param alg 算法
 * @return
 */
QByteArray getDataHash(const QByteArray& data, const HashAlg alg)
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

/**
 * @brief getHashSize 获取哈希算法计算后的哈希长度（Byte）
 * @param alg 哈希算法
 * @return
 */
size_t getHashSize(const HashAlg alg)
{
    return 0;
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
