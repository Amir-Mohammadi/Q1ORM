#include "applicationdbcontext.h"
#include "Mapping/CityMap.h"
#include "Mapping/CountryMap.h"

ApplicationDbContext::ApplicationDbContext(Q1Connection* conn)
    : cities(conn),
      countries(conn)
{
    connection = conn;
}


void ApplicationDbContext::OnConfiguration()
{
    if(!connection)
        connection = new Q1Connection(Q1Driver::POSTGRE_SQL, "localhost", "solo", "postgres", "123");
}

QList<Q1Table> ApplicationDbContext::OnTablesCreating()
{

    CityMap::ConfigureEntity(cities);
    CountryMap::ConfigureEntity(countries);

    return { cities.GetTable(),countries.GetTable()};
}

QList<Q1Relation> ApplicationDbContext::OnTableRelationCreating()
{

    QList<Q1Relation> relations;

    relations += CityMap::CreateRelations(cities);


    return {relations};
}


