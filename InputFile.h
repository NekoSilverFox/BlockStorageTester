#ifndef INPUTFILE_H
#define INPUTFILE_H

#include <QObject>
#include <QString>
#include <QFile>
#include <QFileInfo>
#include <QDataStream>

/**
 * @brief 输入文件
 */
class InputFile : public QObject
{
    Q_OBJECT
public:
    explicit InputFile(QObject *parent = nullptr, const QString& filePath = nullptr);
    ~InputFile();

    bool setFile(const QString& filePath);
    bool isOpen();
    QDataStream& sin();
    QByteArray read(const qint64 maxlen);
    bool atEnd();
    qint64 fileSize();
    QString filePath();
    QString lastLog();

private:
    QString _last_log;
    QFile _file;
    QFileInfo _info;
    QDataStream _sin;
};

#endif // INPUTFILE_H
