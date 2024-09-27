#ifndef DATABASESERVICE_H
#define DATABASESERVICE_H

#include "BlockInfo.h"

#include <QObject>
#include <QSqlDatabase>

#define DEFAULT_DB_CONN     ""

class DatabaseService : public QObject
{
    Q_OBJECT
public:
    explicit DatabaseService(QObject *parent = nullptr);
    ~DatabaseService();

    /* 数据库相关操作 */
    bool connectDatabase(const QString& host, const int port, const QString& driver,
                         const QString& user, const QString& pwd, const QString& database);
    bool isDatabaseOpen();
    bool isDatabaseExist(const QString& database);
    bool createDatabase(const QString& database);
    bool dropCurDatabase();
    bool disconnectCurDatabase();

    /* 表相关操作 */
    bool createBlockInfoTable(const QString& tbName);
    bool deleteTable(const QString& tbName);
    bool isTableExists(const QString& tbName);
    bool insertNewBlockInfoRow(const QString& tbName, const QByteArray& blockHash,
                               const QString& sourceFilePath, const int blockLoc, const int blockSize);
    int getHashRepeatTimes(const QString& tbName, const QByteArray& blockHash);
    bool updateCounter(const QString& tbName, const QByteArray& blockHash, int count);
    int getTableRowCount(const QString& tbName);
    BlockInfo getBlockInfo(const QString& tbName, const QByteArray& blockHash);

    /* getter 方法*/
    QString getHost();
    qint16  getPort();
    QString getDriver();
    QString getUserName();
    QString getPassword();
    QString getNameDatabase();

    /* 日志相关 */
    QString lastSQL();
    QString lastLog();

private:
    QSqlDatabase _db;   // 连接 & 管理的数据库对象

    QString _host;
    qint16  _port;
    QString _driver;
    QString _user;
    QString _password;
    QString _name_db;

    QString _last_sql;  // 最后执行的 SQL 语句
    QString _last_log;  // 最后记录的日志消息

};

#endif // DATABASESERVICE_H
