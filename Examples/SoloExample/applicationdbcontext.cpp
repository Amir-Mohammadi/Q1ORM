#include "applicationdbcontext.h"
#include "Mapping/CityMap.h"
#include "Mapping/CountryMap.h"
#include <QByteArray>
#include <QtGlobal>

namespace
{
QString ReadEnv(const char* name, const QString& fallback)
{
    const QByteArray value = qgetenv(name);
    return value.isEmpty() ? fallback : QString::fromLocal8Bit(value);
}

int ReadEnvInt(const char* name, int fallback)
{
    bool ok = false;
    const int value = ReadEnv(name, QString::number(fallback)).toInt(&ok);
    return ok ? value : fallback;
}

Q1Driver ReadDriver()
{
    const QString driver = ReadEnv("Q1ORM_DB_DRIVER", "postgres").toLower();
    return driver.contains("sqlserver") || driver.contains("mssql") || driver.contains("odbc")
               ? Q1Driver::SQLSERVER
               : Q1Driver::POSTGRE_SQL;
}

int DefaultPort(Q1Driver driver)
{
    return driver == Q1Driver::SQLSERVER ? 1433 : 5432;
}
}

ApplicationDbContext::ApplicationDbContext(Q1Connection* conn)
    : cities(conn),
    countries(conn)
{
    SetConnection(conn, false);
}

void ApplicationDbContext::OnConfiguration()
{
    if (!connection)
    {
        const Q1Driver driver = ReadDriver();
        SetConnection(new Q1Connection(
            driver,
            ReadEnv("Q1ORM_DB_HOST", "localhost"),
            ReadEnv("Q1ORM_DB_NAME", "q1orm_test"),
            ReadEnv("Q1ORM_DB_USER", "postgres"),
            ReadEnv("Q1ORM_DB_PASSWORD", "123"),
            ReadEnvInt("Q1ORM_DB_PORT", DefaultPort(driver))
            ), true);
    }
}

QList<Q1Table*> ApplicationDbContext::OnTablesCreating()
{
    CityMap::ConfigureEntity(cities);
    CountryMap::ConfigureEntity(countries);
    CityMap::CreateRelations(cities);
    CountryMap::CreateRelations(countries);

    QList<Q1Table*> tables;
    tables.append(cities.GetTablePtr());
    tables.append(countries.GetTablePtr());

    return tables;
}

QList<Q1Relation> ApplicationDbContext::OnTableRelationCreating()
{
    QList<Q1Relation> relations;

    relations += CityMap::CreateRelations(cities);

    return relations;
}
