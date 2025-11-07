#include <QCoreApplication>
#include <QDebug>
#include "Services/InsertService.h"
#include "applicationdbcontext.h"

#include "Q1ORM.h"

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    Q1Connection* conn = new Q1Connection(
        Q1Driver::POSTGRE_SQL,
        "localhost",
        "solo",
        "postgres",
        "123"
        );

    if (!conn->RootConnect() || !conn->Connect())
    {
        qDebug() << "Database connection failed.";
        return 1;
    }

    // Initialize ORM Context
    ApplicationDbContext ctx(conn);
    ctx.Initialize();

    // Create InsertService
    InsertService service(ctx.cities, ctx.countries);

    // // Insert Country
    // Country usa;
    // usa.name = "USA";
    // service.AddCountry(usa);

    // // Insert City
    // City ny;
    // ny.name = "New York";
    // ny.country_id = usa.id; // auto-filled after insert
    // service.AddCity(ny);

    // Get all countries
    auto countries = service.GetAllcountries();

    // Join countries and cities
    auto countriesWithcities = service.GetcountriesWithcities();

    qDebug() << "=== countries with cities ===";
    for (auto& c : countriesWithcities) {
        qDebug() << c.id << c.name;
    }

    conn->Disconnect();
    conn->RootDisconnect();

    return a.exec();
}
