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

struct Hash
{
    static QByteArray getDataHash(const QByteArray& data, HashAlg alg);
    static QString getHashName(const HashAlg alg);
    static size_t getHashSize(const HashAlg alg);
};

#endif // HASHALGORITHM_H
