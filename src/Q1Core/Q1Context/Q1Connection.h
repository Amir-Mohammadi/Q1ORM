#ifndef Q1CONNECTION_H
#define Q1CONNECTION_H

#include <QSqlError>
#include <QSqlDatabase>
#include <QString>
#include <QStringList>
#include <QDateTime>
#include <QUuid>
#include <qdebug.h>

#include "../../Q1ORM_global.h"

                  enum Q1Driver
{
    POSTGRE_SQL
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
        root = QSqlDatabase::addDatabase(driver_name, "root-" + name);

        database.setHostName(this->host_name);
        database.setPort(this->port);
        database.setDatabaseName(this->database_name);
        database.setUserName(this->username);
        database.setPassword(this->password);

        root.setHostName(this->host_name);
        root.setPort(this->port);
        root.setDatabaseName(default_databases[driver]);
        root.setUserName(this->username);
        root.setPassword(this->password);
    }

public: //Setter
    void SetDriver(Q1Driver driver)
    {
        this->driver = driver;

        driver_name = drivers[driver];
        port = ports[driver];
    }

    void SetHostName(QString host_name)
    {
        this->host_name = host_name;
        database.setHostName(host_name);
    }

    void SetPort(int port)
    {
        this->port = port;
        database.setPort(port);
    }

    void SetDatabaseName(QString database_name)
    {
        this->database_name = database_name;
        database.setDatabaseName(database_name);
    }

    void SetUsername(QString username)
    {
        this->username = username;
        database.setUserName(username);
    }

    void SetPassword(QString password)
    {
        this->password = password;
        database.setPassword(password);
    }

public: //Getter
    Q1Driver GetDrvier()
    {
        return driver;
    }

    QString GetHostName()
    {
        return host_name;
    }

    int GetPort()
    {
        return port;
    }

    QString GetDatabaseName()
    {
        return database_name;
    }

    QString GetUsername()
    {
        return this->username;
    }

public: //Error
    QString ErrorMessage()
    {
        if(error_type == QSqlError::ErrorType::NoError)
        {
            return "";
        }

        return error.databaseText();
    }

    QSqlError::ErrorType ErrorType()
    {
        return error_type;
    }

public:
    bool RootConnect()
    {
        bool is_open = root.open();

        if(!is_open)
        {
            error = root.lastError();
            error_type = error.type();
        }

        return is_open;
    }

    void RootDisconnect()
    {
        root.close();
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
    QSqlDatabase root;

    QSqlError::ErrorType error_type = QSqlError::ErrorType::NoError;
    QSqlError error;

private: //Connection Parameters
    Q1Driver driver;
    QString driver_name;

    QString host_name;
    int port;

    QString name = "conn_" + QUuid::createUuid().toString().remove('{').remove('}').remove('-');
    QString database_name;
    QString username;
    QString password;

    bool is_open = false;

private: //Defaults
    QStringList default_databases = {"postgres"};
    QStringList drivers = {"QPSQL"};
    QList<int> ports = {5432};
};

#endif // Q1CONNECTION_H


