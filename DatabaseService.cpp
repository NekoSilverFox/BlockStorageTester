#include "DatabaseService.h"

#include <QSqlQuery>
#include <QSqlError>

DatabaseService::DatabaseService(QObject *parent)
    : QObject{parent}
{
    _host = "localhost";
    _port = 5432;
    _driver = "QPSQL";
    _user = "";
    _password = "";
    _name_db = "";
}


DatabaseService::~DatabaseService()
{
    disconnectCurDatabase();
}


// MacOS 下 PostgreSQL 连接问题解决：https://ru.stackoverflow.com/questions/1478871/qpsql-driver-not-found
/**
 * @brief DatabaseService::connectDatabase 连接到指定数据库
 * @param host 主机名或链接
 * @param port 端口号
 * @param driver Qt 数据库驱动
 * @param user 用户名
 * @param pwd 密码
 * @param database 数据库名称
 * @return 是否成功连接
 */
bool DatabaseService::connectDatabase(const QString& host, const int port, const QString& driver,
                                      const QString& user, const QString& pwd, const QString& database)
{
    _host = host;
    _port = port;
    _driver = driver;
    _user = user;
    _password = pwd;
    _name_db = database;

    _db = QSqlDatabase::addDatabase(_driver, QSqlDatabase::defaultConnection);
    _db.setHostName(_host);
    _db.setPort(_port);
    _db.setUserName(_user);
    _db.setPassword(_password);

    if (!_name_db.isEmpty())
    {

        _db.setDatabaseName(_name_db);
    }

    if (_db.open())
    {
        _last_msg = QString("Successed connect to database %1 from %2:%3").arg(_name_db, _host, QString::number(_port));
        return true;
    }
    else
    {
        _last_msg = QString("Cannot connect to datebase %1 from %2:%3").arg(_name_db, _host, QString::number(_port));
        return false;
    }
}


/**
 * @brief DatabaseService::isDatabaseOpen 当前数据库是否连接
 * @return
 */
bool DatabaseService::isDatabaseOpen()
{
    if (_db.isValid() && _db.open())
    {
        _last_msg = "Database connected successfully!";
        return true;
    }
    else
    {
        // 如果连接失败，输出错误信息
        _last_msg = QString("Database connection failed: %1").arg(_db.lastError().text());
        return false;
    }
}


/**
 * @brief DatabaseService::isDatabaseExist 给定数据库是否存在
 * @param database 数据库名称
 * @return true - 存在；false - 数据库未连接或者不存在
 */
bool DatabaseService::isDatabaseExist(const QString& database)
{
    if (!isDatabaseOpen())
    {
        return false;
    }

    QString sql = QString("SELECT 1 FROM pg_database WHERE datname = '%1';").arg(database);
    QSqlQuery q(sql);
    qDebug() << QString("Run SQL: %1").arg(sql);

    q.next();
    if (q.value(0) == 1)
    {
        _last_msg = QString("Database `%1` exist").arg(database);
        q.clear();
        return true;
    }

    _last_msg = QString("Database `%1` do not exist").arg(database);
    q.clear();
    return false;
}


/**
 * @brief DatabaseService::createDatabase 创建指定数据库
 * @param database 要创建的数据库名
 * @return true - 创建成功；false - 数据库未连接或者未创建成功
 */
bool DatabaseService::createDatabase(const QString& database)
{
    if (!isDatabaseOpen())
    {
        return false;
    }

    QString sql = QString("CREATE DATABASE \"%1\";").arg(database);
    QSqlQuery q;
    qDebug() << QString("Run SQL: %1").arg(sql);

    bool succ = q.exec(sql);
    if (succ)
    {
        _last_msg  = QString("Successed create database `%1`").arg(database);
        return true;
    }
    else
    {
        _last_msg = QString("Failed to create database `%1`: %2").arg(database, q.lastError().text());
        return false;
    }
}

/**
 * @brief DatabaseService::dropCurDatabase 删除当前正在使用的数据库（）
 * @return true - 删除成功；false - 数据库未连接或者未删除成功或者传入了默认数据库（数据库名字为空）
 */
bool DatabaseService::dropCurDatabase()
{
    if (!isDatabaseOpen())
    {
        return false;
    }

    if (_name_db.isEmpty())
    {
        _last_msg = "Can not remove default database (database name is empty)";
        return false;
    }

    disconnectCurDatabase();

    QString drop_db_name = _name_db;
    connectDatabase(_host, _port, _driver, _user, _password, ""); // 要删除，先连接到默认数据库(注意：这里重新连接至默认数据库，并使 _name_db = "")

    QSqlQuery q;
    QString sql = QString("DROP DATABASE \"%1\";").arg(drop_db_name);
    bool succ = q.exec(sql);
    if (succ)
    {
        _last_msg = QString("Successed drop current database `%1`").arg(drop_db_name);
        return true;
    }
    else
    {
        _last_msg =  QString("Failed drop current database `%1`:, %2").arg(drop_db_name, _db.lastError().text());
        return false;
    }
}


/**
 * @brief DatabaseService::disconnectDatabase 断开与当前数据库的连接
 * @return 数据库未连接或者未打开
 */
bool DatabaseService::disconnectCurDatabase()
{
    // 检查数据库连接是否有效并已打开
    if (_db.isValid() && _db.open())
    {
        _db.close();
        _db = QSqlDatabase(); // 将 _curDB 重置为空，以解除与实际数据库连接的关联
        QSqlDatabase::removeDatabase(QSqlDatabase::defaultConnection);

        _last_msg = QString("Disconnect database %1").arg(_db.databaseName());
        return true;
    }

    _last_msg = QString("Failed disconnect database %1: %2").arg(_db.databaseName(), _db.lastError().text());
    return false;
}

QString DatabaseService::lastMsg()
{
    return _last_msg;
}

QString DatabaseService::getHost()
{
    return _host;
}

qint16  DatabaseService::getPort()
{
    return _port;
}

QString DatabaseService::getDriver()
{
    return _driver;
}

QString DatabaseService::getUserName()
{
    return _user;
}

QString DatabaseService::getPassword()
{
    return _password;
}

QString DatabaseService::getNameDatabase()
{
    return  _name_db;
}

