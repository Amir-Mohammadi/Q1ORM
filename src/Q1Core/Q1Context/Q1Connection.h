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
#include <QtSql/QSqlQuery>

#include "../../Q1ORM_global.h"

enum Q1Driver
{
    POSTGRE_SQL,
    SQLSERVER,
    MYSQL,
    SQLITE
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

        if (IsSqlite() && this->database_name.isEmpty())
        {
            this->database_name = "sqlite_test.db";
        }

        if(port != 0)
        {
            this->port = port;
        }

        RegisterDatabases();
        ApplyConnectionSettings();
    }

    ~Q1Connection()
    {
        UnregisterDatabases();
    }

    Q1Connection(const Q1Connection&) = delete;
    Q1Connection& operator=(const Q1Connection&) = delete;

public: // Setter
    void SetDriver(Q1Driver driver)
    {
        if(this->driver == driver)
        {
            ApplyConnectionSettings();
            return;
        }

        // The QSqlDatabase handle is bound to a driver at creation time,
        // so switching drivers requires re-creating both handles.
        const bool was_open = database.isOpen();
        const bool root_was_open = root_database.isOpen();

        UnregisterDatabases();

        this->driver = driver;
        driver_name = drivers[driver];
        port = ports[driver];

        if (IsSqlite() && this->database_name.isEmpty())
        {
            this->database_name = "sqlite_test.db";
        }

        RegisterDatabases();
        ApplyConnectionSettings();

        if(was_open) Connect();
        if(root_was_open) RootConnect();
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
        if (IsSqlite() && this->database_name.isEmpty())
        {
            this->database_name = "sqlite_test.db";
        }
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

    bool IsMySql() const
    {
        return driver == MYSQL;
    }

    bool IsSqlite() const
    {
        return driver == SQLITE;
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

        if (IsMySql())
        {
            QString escaped = identifier;
            escaped.replace('`', "``");
            return QString("`%1`").arg(escaped);
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

        // databaseText() is often empty for driver-level failures.
        const QString database_text = error.databaseText().trimmed();
        if(!database_text.isEmpty())
            return database_text;

        const QString driver_text = error.driverText().trimmed();
        if(!driver_text.isEmpty())
            return driver_text;


        return error.text();
    }

    QSqlError::ErrorType ErrorType() const
    {
        return error_type;
    }

public:
    bool RootConnect()
    {
        if(root_is_open)
        {
            ++root_open_count;
            root_is_open = true;
            return true;
        }

        root_open_count = 0;
        root_is_open = false;

        if(!EnsureDriverAvailable(root_database))
            return false;

        if(!root_database.open())
        {
            if (IsSqlServer() && TryOpenSqlServerWithFallbacks(root_database, default_databases[driver], true))
            {
                ClearError();
                root_open_count = 1;
                root_is_open = true;
                return true;
            }
            RecordError(root_database.lastError());
            qCritical() << "Q1Connection::RootConnect failed:" << error.text();
            return false;
        }

        ClearError();
        root_open_count = 1;
        root_is_open = true;
        return true;
    }

    void RootDisconnect()
    {
        if(root_open_count > 0)
            --root_open_count;

        if(root_open_count > 0)
            return;

        CloseHandle(root_database);
        root_is_open = false;
    }

    bool IsOpen() const
    {
        return database.isOpen();
    }

    bool IsRootOpen() const
    {
        return root_database.isOpen();
    }

    bool Connect()
    {
        // Already open: just take a reference, so a nested Disconnect()
        // from an inner scope cannot close a connection an outer scope owns.
        if (database.isOpen())
        {
            ++open_count;
            is_open = true;
            return true;
        }

        open_count = 0;
        is_open = false;
        transaction_depth = 0;
        rollback_requested = false;

        if (!EnsureDriverAvailable(database))
            return false;

        if (!database.open())
        {
            if (IsSqlServer() && TryOpenSqlServerWithFallbacks(database, database_name, false))
            {
                ClearError();
                open_count = 1;
                is_open = true;
                return true;
            }
            RecordError(database.lastError());
            qCritical() << "Q1Connection::Connect failed:" << error.text();
            return false;
        }

        ClearError();

        if (!ApplySessionSettings())
        {
            database.close();
            return false;
        }

        open_count = 1;
        is_open = true;
        return true;
    }

    void Disconnect()
    {
        if (open_count > 0)
            --open_count;

        // Still referenced by an outer scope.
        if (open_count > 0)
            return;

        // An open transaction pins the connection until commit/rollback.
        if (transaction_depth > 0)
        {
            open_count = 1;
            return;
        }

        CloseHandle(database);
        is_open = false;
    }

    bool BeginTransaction()
    {
        const bool was_open = database.isOpen();

        if(!was_open && !Connect())
            return false;

        // Nested begin: join the ambient transaction instead of failing.
        if(transaction_depth > 0)
        {
            ++transaction_depth;
            if(was_open)
                ++open_count;
            return true;
        }

        if (!database.transaction())
        {
            RecordError(database.lastError());
            qCritical() << "Q1Connection::BeginTransaction failed:" << error.text();
            if (!was_open) Disconnect();
            return false;
        }

        ClearError();
        transaction_depth = 1;
        rollback_requested = false;

        // Pin the connection for the lifetime of the transaction.
        if (was_open) ++open_count;

        return true;
    }

    bool CommitTransaction()
    {
        // No tracked transaction (e.g. database.transaction() was used directly).
        if (transaction_depth == 0)
        {
            if (database.commit())
            {
                ClearError();
                return true;
            }
            RecordError(database.lastError());
            return false;
        }

        // Inner scope: the outermost commit decides the real outcome.
        if (transaction_depth > 1)
        {
            --transaction_depth;
            ReleaseTransactionReference();
            return !rollback_requested;
        }

        bool succeeded;

        if (rollback_requested)
        {
            database.rollback();
            succeeded = false;
            qCritical() << "Q1Connection::CommitTransaction rolled back:"
                        << "an inner operation requested a rollback";
        }
        else
        {
            succeeded = database.commit();
            if (succeeded)
                ClearError();
            else
            {
                RecordError(database.lastError());
                database.rollback();
                qCritical() << "Q1Connection::CommitTransaction failed:" << error.text();
            }
        }

        transaction_depth = 0;
        rollback_requested = false;
        ReleaseTransactionReference();
        return succeeded;
    }

    bool RollbackTransaction()
    {
        if (transaction_depth == 0)
        {
            if (database.rollback())
            {
                ClearError();
                return true;
            }
            RecordError(database.lastError());
            return false;
        }

        // Inner scope: mark the whole transaction as doomed.
        if (transaction_depth > 1)
        {
            --transaction_depth;
            rollback_requested = true;
            ReleaseTransactionReference();
            return true;
        }

        const bool succeeded = database.rollback();
        if (succeeded)
            ClearError();
        else
        {
            RecordError(database.lastError());
            qCritical() << "Q1Connection::RollbackTransaction failed:" << error.text();
        }

        transaction_depth = 0;
        rollback_requested = false;
        ReleaseTransactionReference();
        return succeeded;
    }

public:
    QSqlDatabase database;
    QSqlDatabase root_database;

    QSqlError::ErrorType error_type = QSqlError::ErrorType::NoError;
    QSqlError error;

private: // Connection Parameters

    void RegisterDatabases()
    {
        database = QSqlDatabase::addDatabase(driver_name, name);
        root_database = QSqlDatabase::addDatabase(driver_name, root_name);
    }

    void UnregisterDatabases()
    {
        open_count = 0;
        root_open_count = 0;
        transaction_depth = 0;
        rollback_requested = false;

        CloseHandle(database);
        CloseHandle(root_database);
        is_open = false;
        root_is_open = false;

        // Drop our own copies first, otherwise removeDatabase() warns
        // that the connection is still in use.
        database = QSqlDatabase();
        root_database = QSqlDatabase();

        if(QSqlDatabase::contains(name))
            QSqlDatabase::removeDatabase(name);
        if(QSqlDatabase::contains(root_name))
            QSqlDatabase::removeDatabase(root_name);
    }

    void CloseHandle(QSqlDatabase &db)
    {
        if(db.isValid() && db.isOpen())
            db.close();
    }


    void ReleaseTransactionReference()
    {
        Disconnect();
    }

    bool EnsureDriverAvailable(const QSqlDatabase &db)
    {
        if(db.isValid())
            return true;

        error = QSqlError(QString("driver \"%1\" is not available").arg(driver_name),
                          QString("Qt SQL driver \"%1\" could not be loaded").arg(driver_name),
                          QSqlError::ConnectionError);
        error_type = error.type();
        qCritical() << "Q1Connection: driver not available:" << driver_name
                    << "available drivers:" << QSqlDatabase::drivers();

        return false;
    }


    void RecordError(const QSqlError &sql_error)
    {
        error = sql_error;
        error_type = sql_error.type();
    }

    void ClearError()
    {
        error = QSqlError();
        error_type = QSqlError::ErrorType::NoError;
    }

    void ApplyConnectionSettings()
    {
        ConfigureDatabase(database, database_name);
        ConfigureDatabase(root_database, default_databases[driver]);
    }

    bool ApplySessionSettings()
    {
        if (!IsSqlite())
            return true;

        QSqlQuery pragma(database);
        if (pragma.exec(QStringLiteral("PRAGMA foreign_keys = ON")))
            return true;

        RecordError(pragma.lastError());
        qCritical() << "Q1Connection::Connect failed to enable SQLite foreign keys:"
                    << error.text();
        return false;
    }

    void ConfigureDatabase(QSqlDatabase &db, const QString &target_database_name)
    {
        if (!db.isValid())
            return;

        db.setUserName(username);
        db.setPassword(password);

        if (IsSqlServer())
        {
            const QString odbc_driver_name = SqlServerOdbcDrivers().value(0, QStringLiteral("ODBC Driver 17 for SQL Server"));
            db.setHostName(QString());
            db.setPort(0);
            db.setDatabaseName(BuildSqlServerConnectionString(target_database_name, odbc_driver_name));
            return;
        }

        db.setHostName(host_name);
        db.setPort(port);
        if (!target_database_name.isEmpty())
            db.setDatabaseName(target_database_name);
    }

    QString BuildSqlServerConnectionString(const QString &target_database_name,
                                           const QString &odbc_driver_name) const
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

        return QString("Driver={%1};Server=%2;Database=%3;")
            .arg(odbc_driver_name, server, target_database_name);
    }

    QStringList SqlServerOdbcDrivers() const
    {
        QStringList driversToTry;

        if (qEnvironmentVariableIsSet("Q1ORM_SQLSERVER_ODBC_DRIVER"))
            driversToTry << qEnvironmentVariable("Q1ORM_SQLSERVER_ODBC_DRIVER");

        driversToTry << "ODBC Driver 18 for SQL Server"
                     << "ODBC Driver 17 for SQL Server"
                     << "ODBC Driver 13 for SQL Server"
                     << "SQL Server Native Client 11.0";

        driversToTry.removeAll(QString());
        driversToTry.removeDuplicates();
        return driversToTry;
    }

    bool TryOpenSqlServerWithFallbacks(QSqlDatabase &db,
                                       const QString &target_database_name,
                                       bool is_root)
    {
        const QStringList driversToTry = SqlServerOdbcDrivers();

        for (const QString &odbc_driver_name : driversToTry)
        {
            db.setDatabaseName(BuildSqlServerConnectionString(target_database_name, odbc_driver_name));
            if (db.open())
                return true;
        }

        RecordError(db.lastError());
        qCritical() << (is_root ? "Q1Connection::RootConnect failed:" : "Q1Connection::Connect failed:")
                    << error.text();
        return false;
    }

    Q1Driver driver;
    QString driver_name;

    QString host_name;
    int port;

    QString name = "conn_" + QUuid::createUuid().toString().remove('{').remove('}').remove('-');
    QString database_name;
    QString root_name = "root-" + name;
    QString username;
    QString password;

    bool is_open = false;
    bool root_is_open = false;

    int open_count = 0;
    int root_open_count = 0;
    int transaction_depth = 0;
    bool rollback_requested = false;

private: // Defaults
    QStringList default_databases = {"postgres", "master", "", ""};
    QStringList drivers = {"QPSQL", "QODBC", "QMYSQL", "QSQLITE"};
    QList<int> ports = {5432, 1433, 3306, 0};
};

#endif // Q1CONNECTION_H
