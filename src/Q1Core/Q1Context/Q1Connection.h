#ifndef Q1CONNECTION_H
#define Q1CONNECTION_H

#include <QString>
#include <QStringList>
#include <QDateTime>
#include <QUuid>
#include <QDebug>
#include <QtGlobal>
#include <QtSql/QSqlError>
#include <QtSql/QSqlDatabase>

#include "../../Q1ORM_global.h"

enum Q1Driver
{
    POSTGRE_SQL,
    SQLSERVER
};

class Q1ORM_EXPORT Q1Connection
{
public:
    Q1Connection(Q1Driver driver, QString host_name, QString database_name, QString username, QString password, int port = 0)
    {
        this->driver = driver;
        driver_name = drivers[driver];
        this->port = ports[driver];
        this->host_name = host_name;
        this->database_name = database_name;
        this->username = username;
        this->password = password;

        if(port != 0)
        {
            this->port = port;
        }

        database = QSqlDatabase::addDatabase(driver_name, name);
        root_database = QSqlDatabase::addDatabase(driver_name, "root-" + name);

        ApplyConnectionSettings();
    }

    ~Q1Connection()
    {
        Disconnect();
        RootDisconnect();
    }

public: // Setter
    void SetDriver(Q1Driver driver)
    {
        this->driver = driver;
        driver_name = drivers[driver];
        port = ports[driver];
        ApplyConnectionSettings();
    }

    void SetHostName(QString host_name)
    {
        this->host_name = host_name;
        ApplyConnectionSettings();
    }

    void SetPort(int port)
    {
        this->port = port;
        ApplyConnectionSettings();
    }

    void SetDatabaseName(QString database_name)
    {
        this->database_name = database_name;
        ApplyConnectionSettings();
    }

    void SetUsername(QString username)
    {
        this->username = username;
        ApplyConnectionSettings();
    }

    void SetPassword(QString password)
    {
        this->password = password;
        ApplyConnectionSettings();
    }

public: // Getter
    Q1Driver GetDriver() const
    {
        return driver;
    }

    QString GetDatabaseType() const
    {
        return driver_name;
    }

    bool IsPostgreSql() const
    {
        return driver == POSTGRE_SQL;
    }

    bool IsSqlServer() const
    {
        return driver == SQLSERVER;
    }

    QString GetHostName() const
    {
        return host_name;
    }

    int GetPort() const
    {
        return port;
    }

    QString GetDatabaseName() const
    {
        return database_name;
    }

    QString GetUsername() const
    {
        return username;
    }

    QString QuoteIdentifier(const QString &identifier) const
    {
        if (IsSqlServer())
        {
            QString escaped = identifier;
            escaped.replace(']', "]]");
            return QString("[%1]").arg(escaped);
        }

        QString escaped = identifier;
        escaped.replace('"', "\"\"");
        return QString("\"%1\"").arg(escaped);
    }

    QString QuoteStringLiteral(const QString &value) const
    {
        QString escaped = value;
        escaped.replace('\'', "''");

        if (IsSqlServer())
            return QString("N'%1'").arg(escaped);

        return QString("'%1'").arg(escaped);
    }

public: // Error
    QString ErrorMessage() const
    {
        if(error_type == QSqlError::ErrorType::NoError)
        {
            return "";
        }
        return error.databaseText();
    }

    QSqlError::ErrorType ErrorType() const
    {
        return error_type;
    }

public:
    bool RootConnect()
    {
        if(root_is_open) return true;

        bool is_open = root_database.open();

        if(!is_open)
        {
            error = root_database.lastError();
            error_type = error.type();
            qCritical() << "Q1Connection::RootConnect failed:" << error.text();
            return false;
        }

        root_is_open = true;
        return true;
    }

    void RootDisconnect()
    {
        if(root_is_open)
        {
            root_database.close();
            root_is_open = false;
        }
    }

    bool IsOpen() const
    {
        return is_open;
    }

    bool IsRootOpen() const
    {
        return root_is_open;
    }

    bool Connect()
    {
        if(is_open) return true;

        if (!database.open())
        {
            error = database.lastError();
            error_type = error.type();
            qCritical() << "Q1Connection::Connect failed:" << error.text();
            return false;
        }

        is_open = true;
        return true;
    }

    void Disconnect()
    {
        if(is_open)
        {
            database.close();
            is_open = false;
        }
    }

public:
    QSqlDatabase database;
    QSqlDatabase root_database;

    QSqlError::ErrorType error_type = QSqlError::ErrorType::NoError;
    QSqlError error;

private: // Connection Parameters
    void ApplyConnectionSettings()
    {
        ConfigureDatabase(database, database_name);
        ConfigureDatabase(root_database, default_databases[driver]);
    }

    void ConfigureDatabase(QSqlDatabase &db, const QString &target_database_name)
    {
        db.setUserName(username);
        db.setPassword(password);

        if (IsSqlServer())
        {
            db.setHostName(QString());
            db.setPort(0);
            db.setDatabaseName(BuildSqlServerConnectionString(target_database_name));
            return;
        }

        db.setHostName(host_name);
        db.setPort(port);
        db.setDatabaseName(target_database_name);
    }

    QString BuildSqlServerConnectionString(const QString &target_database_name) const
    {
        QString connection_string = host_name.trimmed();

        if (connection_string.contains("driver=", Qt::CaseInsensitive) ||
            connection_string.contains("dsn=", Qt::CaseInsensitive))
        {
            if (!connection_string.endsWith(';'))
                connection_string.append(';');

            if (!target_database_name.isEmpty() &&
                !connection_string.contains("database=", Qt::CaseInsensitive) &&
                !connection_string.contains("initial catalog=", Qt::CaseInsensitive))
            {
                connection_string.append(QString("Database=%1;").arg(target_database_name));
            }

            return connection_string;
        }

        QString server = host_name;
        if (port > 0 && !server.contains(',') && !server.contains('\\'))
            server.append(QString(",%1").arg(port));

        QString odbc_driver = qEnvironmentVariableIsSet("Q1ORM_SQLSERVER_ODBC_DRIVER")
                                  ? qEnvironmentVariable("Q1ORM_SQLSERVER_ODBC_DRIVER")
                                  : QString("ODBC Driver 17 for SQL Server");

        return QString("Driver={%1};Server=%2;Database=%3;")
            .arg(odbc_driver, server, target_database_name);
    }

    Q1Driver driver;
    QString driver_name;

    QString host_name;
    int port;

    QString name = "conn_" + QUuid::createUuid().toString().remove('{').remove('}').remove('-');
    QString database_name;
    QString username;
    QString password;

    bool is_open = false;
    bool root_is_open = false;

private: // Defaults
    QStringList default_databases = {"postgres", "master"};
    QStringList drivers = {"QPSQL", "QODBC"};
    QList<int> ports = {5432, 1433};
};

#endif // Q1CONNECTION_H
