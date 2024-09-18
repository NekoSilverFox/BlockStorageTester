#ifndef DATABASESERVICE_H
#define DATABASESERVICE_H

#include <QObject>
#include <QSqlDatabase>

class DatabaseService : public QObject
{
    Q_OBJECT
public:
    explicit DatabaseService(QObject *parent = nullptr);
    ~DatabaseService();

    bool connectDatabase(const QString& host, const int port, const QString& driver,
                         const QString& user, const QString& pwd, const QString& database);
    bool isDatabaseOpen();
    bool isDatabaseExist(const QString& database);
    bool createDatabase(const QString& database);
    bool dropCurDatabase();
    bool disconnectCurDatabase();

    QString getHost();
    qint16  getPort();
    QString getDriver();
    QString getUserName();
    QString getPassword();
    QString getNameDatabase();

    QString lastMsg();

signals:

private:
    QSqlDatabase _db;   // 连接 & 管理的数据库对象

    QString _host;
    qint16  _port;
    QString _driver;
    QString _user;
    QString _password;
    QString _name_db;

    QString _last_msg;  // 最后记录的日志消息

};

#endif // DATABASESERVICE_H
