#include "applicationdbcontext.h"
#include "Mapping/CityMap.h"
#include "Mapping/ClubMap.h"
#include "Mapping/CountryMap.h"
#include "Mapping/CourtMap.h"
#include "Mapping/UserMap.h"
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
    const QString driver = ReadEnv("Q1ORM_DB_DRIVER", "sqlite").toLower();
    if (driver.contains("sqlserver") || driver.contains("mssql") || driver.contains("odbc"))
        return Q1Driver::SQLSERVER;
    else if (driver.contains("mysql"))
        return Q1Driver::MYSQL;
    else if (driver.contains("postgres") || driver.contains("pgsql"))
        return Q1Driver::POSTGRE_SQL;
    return Q1Driver::SQLITE;
}

int DefaultPort(Q1Driver driver)
{
    if (driver == Q1Driver::SQLSERVER) return 1433;
    if (driver == Q1Driver::MYSQL) return 3306;
    if (driver == Q1Driver::POSTGRE_SQL) return 5432;
    return 0;
}
}

ApplicationDbContext::ApplicationDbContext(Q1Connection* conn)
    : cities(conn),
    clubs(conn),
    countries(conn),
    courts(conn),
    users(conn)
{
    SetConnection(conn, false);
}

void ApplicationDbContext::OnConfiguration()
{
    if (!connection)
    {
        const Q1Driver driver = ReadDriver();
        if (driver == Q1Driver::SQLITE)
        {
            const QString dbPath = ReadEnv("Q1ORM_DB_PATH", "sqlite_test.db");
            SetConnection(new Q1Connection(driver, QString(), dbPath, QString(), QString(), 0), true);
        }
        else
        {
            SetConnection(new Q1Connection(
                driver,
                ReadEnv("Q1ORM_DB_HOST", "localhost"),
                ReadEnv("Q1ORM_DB_NAME", "test_db"),
                ReadEnv("Q1ORM_DB_USER", "root"),
                ReadEnv("Q1ORM_DB_PASSWORD", ""),
                ReadEnvInt("Q1ORM_DB_PORT", DefaultPort(driver))
                ), true);
        }
    }
}

QList<Q1Table*> ApplicationDbContext::OnTablesCreating()
{
    CityMap::ConfigureEntity(cities);
    ClubMap::ConfigureEntity(clubs);
    CountryMap::ConfigureEntity(countries);
    CourtMap::ConfigureEntity(courts);
    UserMap::ConfigureEntity(users);

    QList<Q1Table*> tables;
    tables.append(cities.GetTablePtr());
    tables.append(clubs.GetTablePtr());
    tables.append(countries.GetTablePtr());
    tables.append(courts.GetTablePtr());
    tables.append(users.GetTablePtr());

    return tables;
}

QList<Q1Relation> ApplicationDbContext::OnTableRelationCreating()
{
    QList<Q1Relation> relations;

    relations += CityMap::CreateRelations(cities);
    relations += ClubMap::CreateRelations(clubs);
    relations += CountryMap::CreateRelations(countries);
    relations += CourtMap::CreateRelations(courts);
    relations += UserMap::CreateRelations(users);

    return relations;
}