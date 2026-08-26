#include <QCoreApplication>
#include <QProcess>
#include <QtTest/QtTest>

#include "Q1ORMTests.h"
#include "SqlGenerationTests.h"

namespace
{
struct DatabaseConfig
{
    Q1Driver driver;
    const char* prefix;
    const char* defaultUser;
    const char* defaultPassword;
    const char* defaultDatabase;
    int defaultPort;
};

const DatabaseConfig databases[] = {
    {Q1Driver::POSTGRE_SQL, "PG", "postgres", "123", "q1orm_test", 5432},
    {Q1Driver::SQLSERVER, "SQLSERVER", "sa", "123", "q1orm_test", 1433},
    {Q1Driver::MYSQL, "MYSQL", "root", "", "q1orm_test2", 3306},
    {Q1Driver::SQLITE, "SQLITE", "", "", "q1orm_test.sqlite", 0}
};

QString ReadEnv(const char* name, const QString& fallback)
{
    const QByteArray value = qgetenv(name);
    return value.isEmpty() ? fallback : QString::fromLocal8Bit(value);
}

int ReadPort(const char* name, int fallback)
{
    bool ok = false;
    const int value = ReadEnv(name, QString::number(fallback)).toInt(&ok);
    return ok ? value : fallback;
}

int RunQtTest(QObject* test)
{
    int testArgc = 1;
    char executableName[] = "UnitTestExample";
    char* testArgv[] = {executableName, nullptr};
    return QTest::qExec(test, testArgc, testArgv);
}

int FindDatabase(const QString& prefix)
{
    for (int index = 0; index < static_cast<int>(std::size(databases)); ++index)
    {
        if (prefix.compare(QString::fromLatin1(databases[index].prefix), Qt::CaseInsensitive) == 0)
            return index;
    }
    return -1;
}

int RunDatabaseTest(const DatabaseConfig& database)
{
    const QString prefix = QStringLiteral("Q1ORM_%1_").arg(database.prefix);
    const QString host = ReadEnv((prefix + "HOST").toLocal8Bit().constData(),
                                 ReadEnv("Q1ORM_DB_HOST", "localhost"));
    const QString databaseName = ReadEnv((prefix + "DB_NAME").toLocal8Bit().constData(),
                                         ReadEnv("Q1ORM_TEST_DB_NAME", database.defaultDatabase));
    const QString username = ReadEnv((prefix + "USER").toLocal8Bit().constData(),
                                     ReadEnv("Q1ORM_DB_USER", database.defaultUser));
    const QString password = ReadEnv((prefix + "PASSWORD").toLocal8Bit().constData(),
                                     ReadEnv("Q1ORM_DB_PASSWORD", database.defaultPassword));
    const int port = ReadPort((prefix + "PORT").toLocal8Bit().constData(),
                              ReadPort("Q1ORM_DB_PORT", database.defaultPort));

    Q1ORMTests tests(database.driver, host, databaseName, username, password, port);
    return RunQtTest(&tests);
}
}

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    if (argc >= 3 && QString::fromLocal8Bit(argv[1]) == QStringLiteral("--database"))
    {
        const int databaseIndex = FindDatabase(QString::fromLocal8Bit(argv[2]));
        return databaseIndex >= 0 ? RunDatabaseTest(databases[databaseIndex]) : 1;
    }

    int status = 0;
    {
        SqlGenerationTests tests;
        status |= RunQtTest(&tests);
    }

    for (const DatabaseConfig& database : databases)
    {
        QProcess process;
        process.setProgram(QCoreApplication::applicationFilePath());
        process.setArguments({"--database", database.prefix});
        process.setProcessChannelMode(QProcess::ForwardedChannels);
        process.start();

        if (!process.waitForStarted() || !process.waitForFinished(-1))
            status |= 1;
        else
            status |= process.exitCode() == 0 ? 0 : 1;
    }

    return status;
}
