#include "SqlServerTests.h"

#include <QByteArray>

namespace
{
QString ReadEnv(const char *name, const QString &fallback = QString())
{
    const QByteArray value = qgetenv(name);
    return value.isEmpty() ? fallback : QString::fromLocal8Bit(value);
}

int ReadEnvInt(const char *name, int fallback)
{
    bool ok = false;
    const int value = ReadEnv(name, QString::number(fallback)).toInt(&ok);
    return ok ? value : fallback;
}
}

Q1Driver SqlServerTests::TestDriver() const
{
    return Q1Driver::SQLSERVER;
}

QString SqlServerTests::DriverLabel() const
{
    return QStringLiteral("SQL Server");
}

QString SqlServerTests::Host() const
{
    return ReadEnv("Q1ORM_SQLSERVER_HOST", ReadEnv("Q1ORM_DB_HOST", "localhost"));
}

QString SqlServerTests::DatabaseName() const
{
    return ReadEnv("Q1ORM_SQLSERVER_DB_NAME", ReadEnv("Q1ORM_TEST_DB_NAME", "q1orm_test"));
}

QString SqlServerTests::Username() const
{
    return ReadEnv("Q1ORM_SQLSERVER_USER", ReadEnv("Q1ORM_DB_USER", "sa"));
}

QString SqlServerTests::Password() const
{
    return ReadEnv("Q1ORM_SQLSERVER_PASSWORD", ReadEnv("Q1ORM_DB_PASSWORD", "123"));
}

int SqlServerTests::Port() const
{
    return ReadEnvInt("Q1ORM_SQLSERVER_PORT", ReadEnvInt("Q1ORM_DB_PORT", 1433));
}
