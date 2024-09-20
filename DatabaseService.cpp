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
        _last_log = QString("Successed connect to database %1 from %2:%3").arg(_name_db, _host, QString::number(_port));
        return true;
    }
    else
    {
        _last_log = QString("Cannot connect to datebase %1 from %2:%3").arg(_name_db, _host, QString::number(_port));
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
        _last_log = "Database connected successfully!";
        return true;
    }
    else
    {
        // 如果连接失败，输出错误信息
        _last_log = QString("Database connection failed: %1").arg(_db.lastError().text());
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
    _last_sql = sql;
    QSqlQuery q(sql);
    qDebug() << QString("Run SQL: %1").arg(sql);

    q.next();
    if (q.value(0) == 1)
    {
        _last_log = QString("Database `%1` exist").arg(database);
        q.clear();
        return true;
    }

    _last_log = QString("Database `%1` do not exist").arg(database);
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
    _last_sql = sql;
    QSqlQuery q;
    qDebug() << QString("Run SQL: %1").arg(sql);

    bool succ = q.exec(sql);
    if (succ)
    {
        _last_log  = QString("Successed create database `%1`").arg(database);
        return true;
    }
    else
    {
        _last_log = QString("Failed to create database `%1`: %2").arg(database, q.lastError().text());
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
        _last_log = "Can not remove default database (database name is empty)";
        return false;
    }

    disconnectCurDatabase();

    QString drop_db_name = _name_db;
    connectDatabase(_host, _port, _driver, _user, _password, ""); // 要删除，先连接到默认数据库(注意：这里重新连接至默认数据库，并使 _name_db = "")

    QSqlQuery q;
    QString sql = QString("DROP DATABASE \"%1\";").arg(drop_db_name);
    _last_sql = sql;
    bool succ = q.exec(sql);
    if (succ)
    {
        _last_log = QString("Successed drop current database `%1`").arg(drop_db_name);
        return true;
    }
    else
    {
        _last_log =  QString("Failed drop current database `%1`:, %2").arg(drop_db_name, _db.lastError().text());
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
    if (isDatabaseOpen())
    {
        _db.close();
        _db = QSqlDatabase(); // 将 _curDB 重置为空，以解除与实际数据库连接的关联
        QSqlDatabase::removeDatabase(QSqlDatabase::defaultConnection);

        _last_log = QString("Disconnect database %1").arg(_db.databaseName());
        return true;
    }

    _last_log = QString("Failed disconnect database %1: %2").arg(_db.databaseName(), _db.lastError().text());
    return false;
}

/**
 * @brief DatabaseService::createTable 在当前连接的数据库中创建指定名称的块信息表
 * @param tbName 要创建的表名
 * @return 是否成功创建
 */
bool DatabaseService::createBlockInfoTable(const QString& tbName)
{
    if (!isDatabaseOpen())
    {
        return false;
    }

    QSqlQuery q;
    /**
     * block_hash           块的哈希值
     * source_file_path     块所在的源文件 TODO 确认这里都改了
     * block_loc            块在源文件中的位置 location（在第几字节开始）
     * block_size           块的大小（Byte）
     * counter              块的重复次数（计数器）
     */
    QString sql = QString("CREATE TABLE %1 ("
                          "block_hash BYTEA NOT NULL,"
                          "source_file_path TEXT NOT NULL,"
                          "block_loc NUMERIC(1000, 0) NOT NULL,"
                          "block_size INTEGER NOT NULL,"
                          "counter INTEGER NOT NULL DEFAULT 1);").arg(tbName);
    _last_sql = sql;
    qDebug() << QString("Create table `%1`").arg(tbName);
    qDebug() << QString("↳ Run SQL: %1").arg(sql);

    if (q.exec(sql))
    {
        _last_log = QString("Successed create table `%1`").arg(tbName);
        return true;
    }

    _last_log = QString("Failed to create table `%1`: %2").arg(tbName, q.lastError().text());
    return false;
}


/**
 * @brief DatabaseService::insertNewBlockInfoRow 插入新的块信息行
 * @param tbName 表名
 * @param blockHash 块的哈希值
 * @param sourceFilePath 源文件路径
 * @param blockLoc 块在源文件中的位置（第几字节）
 * @param blockSize 块的大小（Byte）
 * @return
 */
bool DatabaseService::insertNewBlockInfoRow(const QString& tbName, const QByteArray& blockHash,
                                            const QString& sourceFilePath, const int blockLoc, const int blockSize) ///TODO 注意这里参数位置变了
{
    if (!isDatabaseOpen())
    {
        return false;
    }

    // 创建查询对象
    QSqlQuery q(QSqlDatabase::database(QSqlDatabase::defaultConnection));

    // 准备插入语句，使用参数绑定防止 SQL 注入
    QString sql = QString("INSERT INTO %1 (block_hash, source_file_path, block_loc, block_size, counter) "
                          "VALUES (:block_hash, :source_file_path, :block_loc, :block_size, :counter)").arg(tbName);
    _last_sql = sql;
    q.prepare(sql);

    // 绑定参数
    q.bindValue(":block_hash", blockHash);  // 直接绑定 QByteArray
    q.bindValue(":source_file_path", sourceFilePath);
    q.bindValue(":block_loc", blockLoc);
    q.bindValue(":block_size", blockSize);
    q.bindValue(":counter", 1);

    // 执行插入
    if (q.exec())
    {
        _last_log = QString("Successed insert new row to table %1").arg(tbName);
        return true;
    }

    _last_log = QString("Failed to insert new row to table %1: %2").arg(tbName, q.lastError().text());
    return false;
}


/**
 * @brief DatabaseService::getHashRepeatTimes 获取该哈希值在表中的重复次数
 * @param tbName 表名
 * @param blockHash 哈希值
 * @return 重复次数：-1 表示查询失败，否则为重复次数
 */
int DatabaseService::getHashRepeatTimes(const QString& tbName, const QByteArray& blockHash)
{
    if (!isDatabaseOpen())
    {
        return false;
    }

    // 创建查询对象
    QSqlQuery q(QSqlDatabase::database(QSqlDatabase::defaultConnection));

    // 准备查询语句，查找相同的哈希值
    QString sql = QString("SELECT counter FROM %1 WHERE block_hash = :block_hash").arg(tbName);
    _last_sql = sql;
    q.prepare(sql);

    // 绑定哈希值参数
    q.bindValue(":block_hash", blockHash);

    // 执行查询
    if (!q.exec()) {
        _last_log = QString("Failed to get HashRepeatTimes: %1").arg(q.lastError().text());
        return -1;  // 返回 -1 表示查询失败
    }

    // 检查是否存在结果
    if (q.next())
    {
        int counter = q.value(0).toInt();  // 获取重复次数
        _last_log = QString("Find the same hash, repeat times: %1").arg(counter);
        return counter;
    }
    else
    {
        _last_log = "Do not have same hash";
        return 0;  // 返回 0 表示没有找到重复的记录
    }
}

/**
 * @brief DatabaseService::updateCounter 更新表的 counter 字段（哈希值重复次数）
 * @param tbName 表名
 * @param blockHash 块的哈希值
 * @param count 新的重复次数
 * @return 是否更新成功
 */
bool DatabaseService::updateCounter(const QString& tbName, const QByteArray &blockHash, int count)
{
    if (!isDatabaseOpen())
    {
        return false;
    }

    QSqlQuery q(QSqlDatabase::database(QSqlDatabase::defaultConnection));

    QString sql = QString("UPDATE %1 SET counter = :counter WHERE block_hash = :block_hash").arg(tbName);
    q.prepare(sql);

    q.bindValue(":counter", count);
    q.bindValue(":block_hash", blockHash);

    // 执行更新
    if (!q.exec()) {
        _last_log = QString("Update failure: %1").arg(q.lastError().text());
        return false;
    }

    // 检查是否有行受影响
    if (q.numRowsAffected() > 0)
    {
        _last_log = QString("Update successful! Number of rows affected: %1").arg(q.numRowsAffected());
        return true;
    }
    else
    {
        _last_log = "No matching hash found, update failed";
        return false;
    }
}


/**
 * @brief DatabaseService::getTableRowCount 获取表有多少条记录
 * @param tbName 表名
 * @return 行数（负数代表异常）
 */
int DatabaseService::getTableRowCount(const QString& tbName)
{
    if (!isDatabaseOpen())
    {
        return -1;
    }
    QString sql = QString("SELECT COUNT(*) FROM %1").arg(tbName);
    QSqlQuery q;

    if (!q.exec(sql))
    {
        _last_log = QString("Failed get row count of table `%1`: %2").arg(tbName, q.lastError().text());
        return -2;
    }

    if (q.next())
    {
        int rowCount = q.value(0).toInt(); // 获取 COUNT(*) 的结果
        _last_log = QString("Successed get row count of table `%1`, COUNT = %2").arg(tbName, rowCount);
        return rowCount;
    }

    return 0;
}


QString DatabaseService::lastSQL()
{
    return _last_sql;
}

QString DatabaseService::lastLog()
{
    return _last_log;
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
