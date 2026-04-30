#include "PostgreSqlTests.h"

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

Q1Driver PostgreSqlTests::TestDriver() const
{
    return Q1Driver::POSTGRE_SQL;
}

QString PostgreSqlTests::DriverLabel() const
{
    return QStringLiteral("PostgreSQL");
}

QString PostgreSqlTests::Host() const
{
    return ReadEnv("Q1ORM_PG_HOST", ReadEnv("Q1ORM_DB_HOST", "localhost"));
}

QString PostgreSqlTests::DatabaseName() const
{
    return ReadEnv("Q1ORM_PG_DB_NAME", ReadEnv("Q1ORM_TEST_DB_NAME", "q1orm_test"));
}

QString PostgreSqlTests::Username() const
{
    return ReadEnv("Q1ORM_PG_USER", ReadEnv("Q1ORM_DB_USER", "postgres"));
}

QString PostgreSqlTests::Password() const
{
    return ReadEnv("Q1ORM_PG_PASSWORD", ReadEnv("Q1ORM_DB_PASSWORD", "123"));
}

int PostgreSqlTests::Port() const
{
    return ReadEnvInt("Q1ORM_PG_PORT", ReadEnvInt("Q1ORM_DB_PORT", 5432));
}
