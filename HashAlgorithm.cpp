#include "HashAlgorithm.h"


/**
 * @brief getDataHash
 * @param data 数据
 * @param alg 算法
 * @return
 */
QByteArray Hash::getDataHash(const QByteArray& data, const HashAlg alg)
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
 * @brief getHashName 获得哈希算法名字字符串
 * @param alg 哈希算法
 * @return
 */
QString Hash::getHashName(const HashAlg alg)
{
    switch (alg) {
    case HashAlg::MD5:
        return "MD5";

    case HashAlg::SHA1:
        return "SHA1";

    case HashAlg::SHA256:
        return "SHA256";

    case HashAlg::SHA512:
        return "SHA512";

    default:
        return "NONE";
        break;
    }
}

/**
 * @brief getHashSize 获取哈希算法计算后的哈希长度（Byte）
 * @param alg 哈希算法
 * @return TODO
 */
size_t Hash::getHashSize(const HashAlg alg)
{
    QByteArray data = "FOOOOOOOOOX";

    switch (alg) {
    case HashAlg::MD5:
        return getDataHash(data, HashAlg::MD5).size();

    case HashAlg::SHA1:
        return getDataHash(data, HashAlg::SHA1).size();

    case HashAlg::SHA256:
        return getDataHash(data, HashAlg::SHA256).size();

    case HashAlg::SHA512:
        return getDataHash(data, HashAlg::SHA512).size();

    default:
        return 0;
        break;
    }
}
