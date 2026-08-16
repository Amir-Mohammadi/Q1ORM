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
    const QString driver = ReadEnv("Q1ORM_DB_DRIVER", "mysql").toLower();
    return driver.contains("sqlserver") || driver.contains("mssql") || driver.contains("odbc")
                ? Q1Driver::SQLSERVER
                : Q1Driver::MYSQL;
}

int DefaultPort(Q1Driver driver)
{
    return driver == Q1Driver::SQLSERVER ? 1433 : 3306;
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
        SetConnection(new Q1Connection(
            driver,
            ReadEnv("Q1ORM_DB_HOST", "localhost"),
            ReadEnv("Q1ORM_DB_NAME", "DoctorPadelDb"),
            ReadEnv("Q1ORM_DB_USER", "root"),
            ReadEnv("Q1ORM_DB_PASSWORD", "P@nd1378"),
            ReadEnvInt("Q1ORM_DB_PORT", DefaultPort(driver))
            ), true);
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