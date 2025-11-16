#include "Q1ORMTests.h"

void Q1ORMTests::initTestCase()
{
    qDebug() << "\n=== Initializing Q1ORM Test Suite ===\n";

    conn = new Q1Connection(
        Q1Driver::POSTGRE_SQL,
        "localhost",
        "solo_test",
        "postgres",
        "123"
        );

    QVERIFY2(conn->Connect(), "Database connection failed");
    qDebug() << "✓ Database connected successfully";

    ctx = new ApplicationDbContext(conn);
    ctx->OnTablesCreating();
    ctx->OnTableRelationCreating();
    ctx->Initialize();

    qDebug() << "✓ Context initialized";

    // Insert test data
    setupTestData();
}

void Q1ORMTests::cleanupTestCase()
{
    qDebug() << "\n=== Cleaning up Test Suite ===\n";

    if (conn)
    {
        conn->Disconnect();
        conn->RootDisconnect();
        delete ctx;
        delete conn;
    }

    qDebug() << "✓ Cleanup completed";
}

void Q1ORMTests::init()
{
    // Called before each test
}

void Q1ORMTests::cleanup()
{
    // Called after each test
}

void Q1ORMTests::setupTestData()
{
    // Clear existing data
    ctx->cities.Delete("id > 0");
    ctx->countries.Delete("id > 0");

    // Insert countries
    Country usa;
    usa.name = "USA";
    ctx->countries.Insert(usa);

    Country canada;
    canada.name = "Canada";
    ctx->countries.Insert(canada);

    // Insert cities
    City ny;
    ny.name = "New York";
    ny.country_id = usa.id;
    ctx->cities.Insert(ny);

    City la;
    la.name = "Los Angeles";
    la.country_id = usa.id;
    ctx->cities.Insert(la);

    City toronto;
    toronto.name = "Toronto";
    toronto.country_id = canada.id;
    ctx->cities.Insert(toronto);

    qDebug() << "✓ Test data inserted";
}

// ============================================================================
// Test 1: Basic Select Operations
// ============================================================================

void Q1ORMTests::test_selectAll()
{
    QList<City> cities = ctx->cities.Select().ToList();

    QVERIFY(!cities.isEmpty());
    QCOMPARE(cities.size(), 3);
    QCOMPARE(cities[0].name, QString("New York"));
}

void Q1ORMTests::test_selectSpecificColumns()
{
    QList<City> cities = ctx->cities.Select({"id", "name"}).ToList();

    QVERIFY(!cities.isEmpty());
    QCOMPARE(cities.size(), 3);
}

void Q1ORMTests::test_selectAllCountries()
{
    QList<Country> countries = ctx->countries.Select().ToList();

    QVERIFY(!countries.isEmpty());
    QCOMPARE(countries.size(), 2);
}

// ============================================================================
// Test 2: Where Clause
// ============================================================================

void Q1ORMTests::test_whereClause_simple()
{
    QList<City> cities = ctx->cities.Select()
    .Where("country_id = 1")
        .ToList();

    QCOMPARE(cities.size(), 2);
    QCOMPARE(cities[0].country_id, 1);
}

void Q1ORMTests::test_whereClause_comparison()
{
    QList<City> cities = ctx->cities.Select()
    .Where("id > 1")
        .ToList();

    QCOMPARE(cities.size(), 2);
}

// ============================================================================
// Test 3: Order By
// ============================================================================

void Q1ORMTests::test_orderByAsc()
{
    QList<City> cities = ctx->cities.Select()
    .OrderByAsc("name")
        .ToList();

    QCOMPARE(cities.size(), 3);
    QCOMPARE(cities[0].name, QString("Los Angeles"));
    QCOMPARE(cities[1].name, QString("New York"));
    QCOMPARE(cities[2].name, QString("Toronto"));
}

void Q1ORMTests::test_orderByDesc()
{
    QList<City> cities = ctx->cities.Select()
    .OrderByDesc("name")
        .ToList();

    QCOMPARE(cities.size(), 3);
    QCOMPARE(cities[0].name, QString("Toronto"));
    QCOMPARE(cities[1].name, QString("New York"));
    QCOMPARE(cities[2].name, QString("Los Angeles"));
}

void Q1ORMTests::test_customOrderBy()
{
    QList<City> cities = ctx->cities.Select()
    .OrderBy("id DESC")
        .ToList();

    QCOMPARE(cities.size(), 3);
    QVERIFY(cities[0].id > cities[1].id);
}

// ============================================================================
// Test 4: Limit
// ============================================================================

void Q1ORMTests::test_limit()
{
    QList<City> cities = ctx->cities.Select()
    .Limit(2)
        .ToList();

    QCOMPARE(cities.size(), 2);
}

void Q1ORMTests::test_limitWithOrderBy()
{
    QList<City> cities = ctx->cities.Select()
    .OrderByAsc("name")
        .Limit(2)
        .ToList();

    QCOMPARE(cities.size(), 2);
    QCOMPARE(cities[0].name, QString("Los Angeles"));
    QCOMPARE(cities[1].name, QString("New York"));
}

// ============================================================================
// Test 5: Aggregate Functions
// ============================================================================

void Q1ORMTests::test_count()
{
    int count = ctx->cities.Select().Count();
    QCOMPARE(count, 3);
}

void Q1ORMTests::test_countWithWhere()
{
    int count = ctx->cities.Select()
    .Where("country_id = 1")
        .Count();
    QCOMPARE(count, 2);
}

void Q1ORMTests::test_max()
{
    int maxId = ctx->cities.Select().Max<int>("id");
    QVERIFY(maxId >= 3);
}

void Q1ORMTests::test_min()
{
    int minId = ctx->cities.Select().Min<int>("id");
    QVERIFY(minId >= 1);
}

void Q1ORMTests::test_sum()
{
    int sum = ctx->cities.Select().Sum<int>("id");
    QVERIFY(sum > 0);
}

void Q1ORMTests::test_avg()
{
    double avg = ctx->cities.Select().Avg<double>("id");
    QVERIFY(avg > 0.0);
}

void Q1ORMTests::test_distinctCount()
{
    int distinctCountries = ctx->cities.Select()
    .Distinct()
        .Count("country_id");
    QCOMPARE(distinctCountries, 2);
}

// ============================================================================
// Test 6: Inner Join
// ============================================================================

void Q1ORMTests::test_innerJoin_basic()
{
    QList<City> cities = ctx->cities.Select()
    .InnerJoin("countries", "cities.country_id = countries.id")
        .ToList();

    QCOMPARE(cities.size(), 3);
}

void Q1ORMTests::test_innerJoin_withAliases()
{
    QList<City> cities = ctx->cities.Select({"cities.id AS city_id",
                                             "cities.name AS city_name",
                                             "countries.name AS country_name"})
                             .InnerJoin("countries", "cities.country_id = countries.id")
                             .ToList();

    QCOMPARE(cities.size(), 3);

    // Verify JSON contains the aliased columns
    QJsonArray json = ctx->cities.GetLastJson();
    QVERIFY(!json.isEmpty());
    QJsonObject firstRow = json[0].toObject();
    QVERIFY(firstRow.contains("city_id"));
    QVERIFY(firstRow.contains("city_name"));
    QVERIFY(firstRow.contains("country_name"));
}

void Q1ORMTests::test_innerJoin_withWhere()
{
    QList<City> cities = ctx->cities.Select({"cities.name AS city",
                                             "countries.name AS country"})
                             .InnerJoin("countries", "cities.country_id = countries.id")
                             .Where("cities.id > 1")
                             .ToList();

    QCOMPARE(cities.size(), 2);
}

void Q1ORMTests::test_innerJoin_withOrderBy()
{
    QList<City> cities = ctx->cities.Select({"cities.name AS city",
                                             "countries.name AS country"})
                             .InnerJoin("countries", "cities.country_id = countries.id")
                             .OrderByDesc("cities.name")
                             .ToList();

    QCOMPARE(cities.size(), 3);

    QJsonArray json = ctx->cities.GetLastJson();
    QJsonObject firstRow = json[0].toObject();
    QCOMPARE(firstRow["city"].toString(), QString("Toronto"));
}

void Q1ORMTests::test_innerJoin_withLimit()
{
    QList<City> cities = ctx->cities.Select({"cities.name AS city",
                                             "countries.name AS country"})
                             .InnerJoin("countries", "cities.country_id = countries.id")
                             .Limit(2)
                             .ToList();

    QCOMPARE(cities.size(), 2);
}

// ============================================================================
// Test 7: Left Join
// ============================================================================

void Q1ORMTests::test_leftJoin()
{
    QList<City> cities = ctx->cities.Select({"cities.id AS city_id",
                                             "cities.name AS city_name",
                                             "countries.name AS country_name"})
                             .LeftJoin("countries", "cities.country_id = countries.id")
                             .ToList();

    QCOMPARE(cities.size(), 3);
}

// ============================================================================
// Test 8: Right Join
// ============================================================================

void Q1ORMTests::test_rightJoin()
{
    QList<Country> countries = ctx->countries.Select({"countries.id AS country_id",
                                                      "countries.name AS country_name",
                                                      "cities.name AS city_name"})
                                   .RightJoin("cities", "countries.id = cities.country_id")
                                   .ToList();

    QVERIFY(!countries.isEmpty());
}

// ============================================================================
// Test 9: Full Join
// ============================================================================

void Q1ORMTests::test_fullJoin()
{
    QList<City> cities = ctx->cities.Select({"cities.name AS city",
                                             "countries.name AS country"})
                             .FullJoin("countries", "cities.country_id = countries.id")
                             .ToList();

    QVERIFY(!cities.isEmpty());
}

// ============================================================================
// Test 10: Group By and Having
// ============================================================================

void Q1ORMTests::test_groupBy()
{
    QList<City> results = ctx->cities.Select({"countries.name AS country",
                                              "COUNT(*) AS city_count"})
                              .InnerJoin("countries", "cities.country_id = countries.id")
                              .GroupBy("countries.name")
                              .ToList();

    QVERIFY(!results.isEmpty());

    QJsonArray json = ctx->cities.GetLastJson();
    QCOMPARE(json.size(), 2); // USA and Canada
}

void Q1ORMTests::test_groupByWithHaving()
{
    QList<City> results = ctx->cities.Select({"countries.name AS country",
                                              "COUNT(*) AS city_count"})
                              .InnerJoin("countries", "cities.country_id = countries.id")
                              .GroupBy("countries.name")
                              .Having("COUNT(*) > 1")
                              .ToList();

    QJsonArray json = ctx->cities.GetLastJson();
    QCOMPARE(json.size(), 1); // Only USA has more than 1 city
}

// ============================================================================
// Test 11: Include (Eager Loading)
// ============================================================================

void Q1ORMTests::test_include_showList()
{
    QList<City> cities = ctx->cities.Select()
    .Include("countries")
        .ToList();

    QCOMPARE(cities.size(), 3);

    QJsonArray json = ctx->cities.GetLastJson();
    QVERIFY(!json.isEmpty());

    QJsonObject firstCity = json[0].toObject();
    QVERIFY(firstCity.contains("countries"));
    QVERIFY(firstCity["countries"].isArray());
}

void Q1ORMTests::test_include_showJson()
{
    QByteArray jsonData = ctx->cities.Select()
    .Include("countries")
        .ToJson();

    QVERIFY(!jsonData.isEmpty());
    QVERIFY(jsonData.contains("countries"));
}

void Q1ORMTests::test_include_withWhere()
{
    QList<City> cities = ctx->cities.Select()
    .Where("id > 1")
        .Include("countries")
        .ToList();

    QCOMPARE(cities.size(), 2);
}

// ============================================================================
// Test 12: Combined Operations
// ============================================================================

void Q1ORMTests::test_whereOrderByLimit()
{
    QList<City> cities = ctx->cities.Select()
    .Where("country_id = 1")
        .OrderByDesc("name")
        .Limit(2)
        .ToList();

    QCOMPARE(cities.size(), 2);
}

void Q1ORMTests::test_joinWhereOrderByLimit()
{
    QList<City> cities = ctx->cities.Select({"cities.name AS city",
                                             "countries.name AS country"})
                             .InnerJoin("countries", "cities.country_id = countries.id")
                             .Where("cities.id > 1")
                             .OrderByAsc("cities.name")
                             .Limit(3)
                             .ToList();

    QCOMPARE(cities.size(), 2);
}

// ============================================================================
// Test 13: JSON Output
// ============================================================================

void Q1ORMTests::test_toJson()
{
    QByteArray json = ctx->cities.Select()
    .Limit(2)
        .ToJson();

    QVERIFY(!json.isEmpty());
    QVERIFY(json.contains("New York") || json.contains("Los Angeles"));

    QJsonDocument doc = QJsonDocument::fromJson(json);
    QVERIFY(doc.isArray());
    QCOMPARE(doc.array().size(), 2);
}

void Q1ORMTests::test_showJson()
{
    QByteArray json = ctx->cities.Select({"cities.name AS city",
                                          "countries.name AS country"})
                          .InnerJoin("countries", "cities.country_id = countries.id")
                          .Limit(2)
                          .ToJson();

    QVERIFY(!json.isEmpty());
    QJsonDocument doc = QJsonDocument::fromJson(json);
    QVERIFY(doc.isArray());
}

// ============================================================================
// Test 14: ToList
// ============================================================================

void Q1ORMTests::test_toList()
{
    QList<City> cities = ctx->cities.Select().ToList();

    QCOMPARE(cities.size(), 3);
    QVERIFY(!cities[0].name.isEmpty());
}

void Q1ORMTests::test_toListWithWhere()
{
    QList<City> cities = ctx->cities.Select()
    .Where("country_id = 1")
        .ToList();

    QCOMPARE(cities.size(), 2);
    for (const City& city : cities)
    {
        QCOMPARE(city.country_id, 1);
    }
}

// ============================================================================
// Test 15: Edge Cases
// ============================================================================

void Q1ORMTests::test_emptyResult()
{
    QList<City> cities = ctx->cities.Select()
    .Where("id = 999999")
        .ToList();

    QVERIFY(cities.isEmpty());
}

void Q1ORMTests::test_nullValues()
{
    // Test handling of null values if your schema allows them
    QList<City> cities = ctx->cities.Select().ToList();
    QVERIFY(!cities.isEmpty());
}

// ============================================================================
// Main Test Runner
// ============================================================================
QTEST_MAIN(Q1ORMTests)
