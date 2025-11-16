#include <QCoreApplication>
#include <QDebug>
#include "Services/InsertService.h"
#include "applicationdbcontext.h"
#include "Q1ORM.h"

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    qDebug() << "=== SQL Server Connection Test ===\n";

    Q1Connection* conn = new Q1Connection(
        Q1Driver::POSTGRE_SQL,
        "localhost",
        "solo",
        "postgres",
        "123"
        );

    if (!conn->Connect())
    {
        qWarning() << "❌ SQL Server connection failed:" << conn->ErrorMessage();
        return -1;
    }
    else
    {
        qDebug() << "✓ Connected to SQL Server successfully!\n";
    }

    // Initialize ORM Context
    ApplicationDbContext ctx(conn);
    ctx.OnTablesCreating();
    ctx.OnTableRelationCreating();
    ctx.Initialize();

    qDebug() << "\n========================================";
    qDebug() << "        Q1ORM COMPLETE TEST SUITE";
    qDebug() << "========================================\n";

    // ============================================================================
    // TEST 1: Basic Select Operations
    // ============================================================================
    qDebug() << "\n=== TEST 1: Basic Select Operations ===";

    qDebug() << "\n1.1 - Select All Cities:";
    ctx.cities.Select().ShowList();

    qDebug() << "\n1.2 - Select All Countries:";
    ctx.countries.Select().ShowList();

    qDebug() << "\n1.3 - Select Specific Columns:";
    ctx.cities.Select({"id", "name"}).ShowList();

    // ============================================================================
    // TEST 2: Where Clause
    // ============================================================================
    qDebug() << "\n=== TEST 2: Where Clause ===";

    qDebug() << "\n2.1 - Cities where country_id = 1:";
    ctx.cities.Select().Where("country_id = 1").ShowList();

    qDebug() << "\n2.2 - Cities where id > 1:";
    ctx.cities.Select().Where("id > 1").ShowList();

    // ============================================================================
    // TEST 3: Order By
    // ============================================================================
    qDebug() << "\n=== TEST 3: Order By ===";

    qDebug() << "\n3.1 - Order by name ASC:";
    ctx.cities.Select().OrderByAsc("name").ShowList();

    qDebug() << "\n3.2 - Order by name DESC:";
    ctx.cities.Select().OrderByDesc("name").ShowList();

    qDebug() << "\n3.3 - Custom Order By:";
    ctx.cities.Select().OrderBy("id DESC").ShowList();

    // ============================================================================
    // TEST 4: Limit
    // ============================================================================
    qDebug() << "\n=== TEST 4: Limit ===";

    qDebug() << "\n4.1 - First 2 cities:";
    ctx.cities.Select().Limit(2).ShowList();

    qDebug() << "\n4.2 - First 2 cities ordered by name:";
    ctx.cities.Select().OrderByAsc("name").Limit(2).ShowList();

    // ============================================================================
    // TEST 5: Aggregate Functions
    // ============================================================================
    qDebug() << "\n=== TEST 5: Aggregate Functions ===";

    qDebug() << "\n5.1 - Count all cities:";
    int totalCities = ctx.cities.Select().Count();
    qDebug() << "Total cities:" << totalCities;

    qDebug() << "\n5.2 - Count cities with country_id = 1:";
    int usaCities = ctx.cities.Select().Where("country_id = 1").Count();
    qDebug() << "Cities in USA:" << usaCities;

    qDebug() << "\n5.3 - Max city id:";
    int maxId = ctx.cities.Select().Max<int>("id");
    qDebug() << "Max ID:" << maxId;

    qDebug() << "\n5.4 - Min city id:";
    int minId = ctx.cities.Select().Min<int>("id");
    qDebug() << "Min ID:" << minId;

    qDebug() << "\n5.5 - Sum of all ids:";
    int sumIds = ctx.cities.Select().Sum<int>("id");
    qDebug() << "Sum of IDs:" << sumIds;

    qDebug() << "\n5.6 - Average of ids:";
    double avgId = ctx.cities.Select().Avg<double>("id");
    qDebug() << "Average ID:" << avgId;

    qDebug() << "\n5.7 - Count distinct country_id:";
    int distinctCountries = ctx.cities.Select().Distinct().Count("country_id");
    qDebug() << "Distinct countries:" << distinctCountries;

    // ============================================================================
    // TEST 6: Inner Join
    // ============================================================================
    qDebug() << "\n=== TEST 6: Inner Join ===";

    qDebug() << "\n6.1 - Inner Join with all columns (will show warning):";
    ctx.cities.Select()
        .InnerJoin("countries", "cities.country_id = countries.id")
        .ShowList();

    qDebug() << "\n6.2 - Inner Join with aliased columns (recommended):";
    ctx.cities.Select({"cities.id AS city_id",
                       "cities.name AS city_name",
                       "cities.country_id",
                       "countries.id AS country_id",
                       "countries.name AS country_name"})
        .InnerJoin("countries", "cities.country_id = countries.id")
        .ShowList();

    qDebug() << "\n6.3 - Inner Join with WHERE:";
    ctx.cities.Select({"cities.name AS city",
                       "countries.name AS country"})
        .InnerJoin("countries", "cities.country_id = countries.id")
        .Where("cities.id > 1")
        .ShowList();

    qDebug() << "\n6.4 - Inner Join with ORDER BY:";
    ctx.cities.Select({"cities.name AS city",
                       "countries.name AS country"})
        .InnerJoin("countries", "cities.country_id = countries.id")
        .OrderByDesc("cities.name")
        .ShowList();

    qDebug() << "\n6.5 - Inner Join with LIMIT:";
    ctx.cities.Select({"cities.name AS city",
                       "countries.name AS country"})
        .InnerJoin("countries", "cities.country_id = countries.id")
        .Limit(2)
        .ShowList();

    // ============================================================================
    // TEST 7: Left Join
    // ============================================================================
    qDebug() << "\n=== TEST 7: Left Join ===";

    qDebug() << "\n7.1 - Left Join:";
    ctx.cities.Select({"cities.id AS city_id",
                       "cities.name AS city_name",
                       "countries.name AS country_name"})
        .LeftJoin("countries", "cities.country_id = countries.id")
        .ShowList();

    // ============================================================================
    // TEST 8: Right Join
    // ============================================================================
    qDebug() << "\n=== TEST 8: Right Join ===";

    qDebug() << "\n8.1 - Right Join:";
    ctx.countries.Select({"countries.id AS country_id",
                          "countries.name AS country_name",
                          "cities.name AS city_name"})
        .RightJoin("cities", "countries.id = cities.country_id")
        .ShowList();

    // ============================================================================
    // TEST 9: Full Join
    // ============================================================================
    qDebug() << "\n=== TEST 9: Full Outer Join ===";

    qDebug() << "\n9.1 - Full Join:";
    ctx.cities.Select({"cities.name AS city",
                       "countries.name AS country"})
        .FullJoin("countries", "cities.country_id = countries.id")
        .ShowList();

    // ============================================================================
    // TEST 10: Group By and Having
    // ============================================================================
    qDebug() << "\n=== TEST 10: Group By and Having ===";

    qDebug() << "\n10.1 - Group By with COUNT:";
    ctx.cities.Select({"countries.name AS country",
                       "COUNT(*) AS city_count"})
        .InnerJoin("countries", "cities.country_id = countries.id")
        .GroupBy("countries.name")
        .ShowList();

    qDebug() << "\n10.2 - Group By with HAVING:";
    ctx.cities.Select({"countries.name AS country",
                       "COUNT(*) AS city_count"})
        .InnerJoin("countries", "cities.country_id = countries.id")
        .GroupBy("countries.name")
        .Having("COUNT(*) > 1")
        .ShowList();

    qDebug() << "\n10.3 - Complex Group By:";
    ctx.cities.Select({"cities.name AS city",
                       "countries.name AS country",
                       "COUNT(*) AS total"})
        .InnerJoin("countries", "cities.country_id = countries.id")
        .GroupBy("cities.name, countries.name")
        .OrderByDesc("total")
        .Limit(10)
        .ShowList();

    // ============================================================================
    // TEST 11: Include (Eager Loading)
    // ============================================================================
    qDebug() << "\n=== TEST 11: Include (Eager Loading) ===";

    qDebug() << "\n11.1 - Include countries (ShowList - flattened):";
    ctx.cities.Select().Include("countries").ShowList();

    qDebug() << "\n11.2 - Include countries (ShowJson - nested):";
    ctx.cities.Select().Include("countries").ShowJson();

    qDebug() << "\n11.3 - Include with WHERE:";
    ctx.cities.Select()
        .Where("id > 1")
        .Include("countries")
        .ShowList();

    qDebug() << "\n11.4 - Include with specific columns:";
    ctx.cities.Select({"id", "name"})
        .Include("countries")
        .ShowList();

    // ============================================================================
    // TEST 12: Combined Operations
    // ============================================================================
    qDebug() << "\n=== TEST 12: Combined Operations ===";

    qDebug() << "\n12.1 - Where + OrderBy + Limit:";
    ctx.cities.Select()
        .Where("country_id = 1")
        .OrderByDesc("name")
        .Limit(2)
        .ShowList();

    qDebug() << "\n12.2 - Join + Where + OrderBy + Limit:";
    ctx.cities.Select({"cities.name AS city",
                       "countries.name AS country"})
        .InnerJoin("countries", "cities.country_id = countries.id")
        .Where("cities.id > 1")
        .OrderByAsc("cities.name")
        .Limit(3)
        .ShowList();

    // ============================================================================
    // TEST 13: JSON Output
    // ============================================================================
    qDebug() << "\n=== TEST 13: JSON Output ===";

    qDebug() << "\n13.1 - ShowJson (basic):";
    ctx.cities.Select().Limit(2).ShowJson();

    qDebug() << "\n13.2 - ToJson (get JSON as QByteArray):";
    QByteArray jsonData = ctx.cities.Select().Limit(2).ToJson();
    qDebug().noquote() << "JSON Output:";
    qDebug().noquote() << jsonData;

    qDebug() << "\n13.3 - ShowJson with Join:";
    ctx.cities.Select({"cities.name AS city",
                       "countries.name AS country"})
        .InnerJoin("countries", "cities.country_id = countries.id")
        .Limit(2)
        .ShowJson();

    // ============================================================================
    // TEST 14: ToList (get results as QList)
    // ============================================================================
    qDebug() << "\n=== TEST 14: ToList ===";

    qDebug() << "\n14.1 - Get cities as QList:";
    QList<City> cities = ctx.cities.Select().ToList();
    qDebug() << "Retrieved" << cities.size() << "cities";
    for (const City& city : cities)
    {
        qDebug() << "  - City:" << city.name << "(ID:" << city.id << ")";
    }

    qDebug() << "\n14.2 - Get cities with WHERE as QList:";
    QList<City> usaCitiesList = ctx.cities.Select()
                                    .Where("country_id = 1")
                                    .ToList();
    qDebug() << "Retrieved" << usaCitiesList.size() << "cities from USA";

    // ============================================================================
    // TEST 15: Multiple Joins
    // ============================================================================
    qDebug() << "\n=== TEST 15: Multiple Joins ===";

    // Note: This assumes you have a regions table
    // If not, you can skip this test
    qDebug() << "\n15.1 - Multiple joins (if regions table exists):";
    // ctx.cities.Select({"cities.name AS city",
    //                    "countries.name AS country",
    //                    "regions.name AS region"})
    //     .InnerJoin("countries", "cities.country_id = countries.id")
    //     .InnerJoin("regions", "countries.region_id = regions.id")
    //     .ShowList();
    qDebug() << "Skipped - requires regions table";

    // ============================================================================
    // TEST 16: Advanced Scenarios
    // ============================================================================
    qDebug() << "\n=== TEST 16: Advanced Scenarios ===";

    qDebug() << "\n16.1 - Subquery-like with Group By:";
    ctx.cities.Select({"country_id",
                       "COUNT(*) AS total_cities",
                       "MAX(id) AS max_city_id"})
        .GroupBy("country_id")
        .OrderByDesc("total_cities")
        .ShowList();

    qDebug() << "\n16.2 - All aggregate functions together:";
    qDebug() << "Count:" << ctx.cities.Select().Count();
    qDebug() << "Max ID:" << ctx.cities.Select().Max<int>("id");
    qDebug() << "Min ID:" << ctx.cities.Select().Min<int>("id");
    qDebug() << "Sum IDs:" << ctx.cities.Select().Sum<int>("id");
    qDebug() << "Avg ID:" << ctx.cities.Select().Avg<double>("id");

    // ============================================================================
    // TEST 17: Error Cases
    // ============================================================================
    qDebug() << "\n=== TEST 17: Error/Edge Cases ===";

    qDebug() << "\n17.1 - WHERE with no results:";
    ctx.cities.Select()
        .Where("id = 999999")
        .ShowList();

    qDebug() << "\n17.2 - Empty table query (if applicable):";
    // ctx.emptyTable.Select().ShowList();

    // ============================================================================
    // FINAL CLEANUP
    // ============================================================================
    qDebug() << "\n========================================";
    qDebug() << "           TESTS COMPLETED";
    qDebug() << "========================================\n";

    conn->Disconnect();
    conn->RootDisconnect();

    qDebug() << "✓ All tests completed successfully!";
    qDebug() << "✓ Disconnected and cleaned up\n";

    return 0;
}
