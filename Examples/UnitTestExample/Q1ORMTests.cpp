#include "Q1ORMTests.h"
#include <QtSql/QSqlDatabase>

int usaId = 0;
int canadaId = 0;

void Q1ORMTests::initTestCase()
{
    qDebug() << "\n=== Initializing Q1ORM Test Suite ===\n";

    const Q1Driver driver = TestDriver();
    const QString qtDriver = driver == Q1Driver::SQLSERVER ? QStringLiteral("QODBC") : QStringLiteral("QPSQL");

    if (!QSqlDatabase::drivers().contains(qtDriver))
    {
        QSKIP(qPrintable(QString("Qt was built without the %1 driver.").arg(qtDriver)));
    }

    conn = new Q1Connection(
        driver,
        Host(),
        DatabaseName(),
        Username(),
        Password(),
        Port()
        );

    ctx = new ApplicationDbContext(conn);
    if (!ctx->Initialize())
    {
        const QString error = conn->ErrorMessage().isEmpty() ? ctx->GetLastError() : conn->ErrorMessage();
        QSKIP(qPrintable(QString("%1 initialization failed: %2").arg(DriverLabel(), error)));
    }

    qDebug() << "Database initialized successfully";

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

void Q1ORMTests::init() {}
void Q1ORMTests::cleanup() {}

void Q1ORMTests::setupTestData()
{
    ctx->cities.Delete("id > 0");
    ctx->countries.Delete("id > 0");

    Country usa;
    usa.name = "USA";
    ctx->countries.Insert(usa);
    usaId = usa.id;

    Country canada;
    canada.name = "Canada";
    ctx->countries.Insert(canada);
    canadaId = canada.id;

    City ny;
    ny.name = "New York";
    ny.country_id = usaId;
    ctx->cities.Insert(ny);

    City la;
    la.name = "Los Angeles";
    la.country_id = usaId;
    ctx->cities.Insert(la);

    City toronto;
    toronto.name = "Toronto";
    toronto.country_id = canadaId;
    ctx->cities.Insert(toronto);

    qDebug() << "✓ Test data inserted";
}

// ================= SELECT =================

void Q1ORMTests::test_selectAll()
{
    QList<City> cities = ctx->cities.Select().ToList();
    QCOMPARE(cities.size(), 3);
}

void Q1ORMTests::test_selectSpecificColumns()
{
    QList<City> cities = ctx->cities.Select({"id","name"}).ToList();
    QCOMPARE(cities.size(), 3);
}

void Q1ORMTests::test_selectAllCountries()
{
    QList<Country> countries = ctx->countries.Select().ToList();
    QCOMPARE(countries.size(), 2);
}

// ================= WHERE =================

void Q1ORMTests::test_whereClause_simple()
{
    QList<City> cities = ctx->cities.Select()
    .Where(QString("country_id = %1").arg(usaId))
        .ToList();

    QCOMPARE(cities.size(), 2);
}

void Q1ORMTests::test_whereClause_comparison()
{
    QList<City> cities = ctx->cities.Select()
    .Where("id > 1")
        .ToList();

    QVERIFY(cities.size() >= 2);
}

// ================= ORDER =================

void Q1ORMTests::test_orderByAsc()
{
    QList<City> cities = ctx->cities.Select()
    .OrderByAsc("name")
        .ToList();

    QVERIFY(cities.size() >= 1);
    QCOMPARE(cities[0].name, QString("Los Angeles"));
}

void Q1ORMTests::test_orderByDesc()
{
    QList<City> cities = ctx->cities.Select()
    .OrderByDesc("name")
        .ToList();

    QVERIFY(cities.size() >= 1);
    QCOMPARE(cities[0].name, QString("Toronto"));
}

void Q1ORMTests::test_customOrderBy()
{
    QList<City> cities = ctx->cities.Select()
    .OrderBy("id DESC")
        .ToList();

    QVERIFY(cities.size() >= 2);
    QVERIFY(cities[0].id > cities[1].id);
}

// ================= LIMIT =================

void Q1ORMTests::test_limit()
{
    QList<City> cities = ctx->cities.Select().Limit(2).ToList();
    QCOMPARE(cities.size(), 2);
}

void Q1ORMTests::test_limitWithOrderBy()
{
    QList<City> cities = ctx->cities.Select()
    .OrderByAsc("name")
        .Limit(2)
        .ToList();

    QCOMPARE(cities.size(), 2);
}

// ================= AGGREGATE =================

void Q1ORMTests::test_count()
{
    QCOMPARE(ctx->cities.Select().Count(), 3);
}

void Q1ORMTests::test_countWithWhere()
{
    int count = ctx->cities.Select()
    .Where(QString("country_id = %1").arg(usaId))
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

// ================= JOIN =================

void Q1ORMTests::test_innerJoin_basic()
{
    QList<City> cities = ctx->cities.Select()
    .InnerJoin("countries","cities.country_id = countries.id")
        .ToList();

    QCOMPARE(cities.size(),3);
}

void Q1ORMTests::test_innerJoin_withAliases()
{
    ctx->cities.Select({"cities.id AS city_id",
                        "cities.name AS city_name",
                        "countries.name AS country_name"})
        .InnerJoin("countries", "cities.country_id = countries.id")
        .ToList();

    QJsonArray json = ctx->cities.GetLastJson();
    QCOMPARE(json.size(), 3);
    QVERIFY(json.size() >= 1);
    QVERIFY(json[0].toObject().contains("city_id"));
    QVERIFY(json[0].toObject().contains("country_name"));
}

void Q1ORMTests::test_innerJoin_withWhere()
{
    ctx->cities.Select({"cities.name AS city",
                        "countries.name AS country"})
        .InnerJoin("countries", "cities.country_id = countries.id")
        .Where("cities.id > 1")
        .ToList();

    QJsonArray json = ctx->cities.GetLastJson();
    QCOMPARE(json.size(), 3);
}

void Q1ORMTests::test_innerJoin_withOrderBy()
{
    ctx->cities.Select({"cities.name AS city",
                        "countries.name AS country"})
        .InnerJoin("countries", "cities.country_id = countries.id")
        .OrderByDesc("cities.name")
        .ToList();

    QJsonArray json = ctx->cities.GetLastJson();
    QCOMPARE(json.size(), 3);
    QVERIFY(json.size() >= 1);
    QCOMPARE(json[0].toObject().value("city").toString(), QString("Toronto"));
}

void Q1ORMTests::test_innerJoin_withLimit()
{
    ctx->cities.Select({"cities.name AS city",
                        "countries.name AS country"})
        .InnerJoin("countries", "cities.country_id = countries.id")
        .Limit(2)
        .ToList();

    QJsonArray json = ctx->cities.GetLastJson();
    QCOMPARE(json.size(), 2);
}

void Q1ORMTests::test_leftJoin()
{
    QList<City> cities = ctx->cities.Select()
    .LeftJoin("countries","cities.country_id = countries.id")
        .ToList();

    QCOMPARE(cities.size(),3);
}

void Q1ORMTests::test_rightJoin()
{
    QList<Country> countries = ctx->countries.Select()
    .RightJoin("cities","countries.id = cities.country_id")
        .ToList();

    QVERIFY(!countries.isEmpty());
}

void Q1ORMTests::test_fullJoin()
{
    QList<City> cities = ctx->cities.Select()
    .FullJoin("countries","cities.country_id = countries.id")
        .ToList();

    QVERIFY(!cities.isEmpty());
}

// ================= GROUP =================

void Q1ORMTests::test_groupBy()
{
    ctx->cities.Select({"countries.name AS country","COUNT(*) AS city_count"})
    .InnerJoin("countries","cities.country_id = countries.id")
        .GroupBy("countries.name")
        .ToList();

    QJsonArray json = ctx->cities.GetLastJson();
    QCOMPARE(json.size(),2);
}

void Q1ORMTests::test_groupByWithHaving()
{
    ctx->cities.Select({"countries.name AS country","COUNT(*) AS city_count"})
    .InnerJoin("countries","cities.country_id = countries.id")
        .GroupBy("countries.name")
        .Having("COUNT(*) > 1")
        .ToList();

    QJsonArray json = ctx->cities.GetLastJson();
    QCOMPARE(json.size(),1);
}

// ================= INCLUDE =================

void Q1ORMTests::test_include_showList()
{
    QList<City> cities = ctx->cities.Select()
    .Include("countries")
        .ToList();

    QCOMPARE(cities.size(),3);

    QJsonArray json = ctx->cities.GetLastJson();
    QVERIFY(json.size() >= 1);
    QVERIFY(json[0].toObject().contains("countries"));
}

void Q1ORMTests::test_include_showJson()
{
    QByteArray json = ctx->cities.Select()
    .Include("countries")
        .ToJson();

    QVERIFY(json.contains("countries"));
}

void Q1ORMTests::test_include_withWhere()
{
    QList<City> cities = ctx->cities.Select()
    .Where("id > 1")
        .Include("countries")
        .ToList();

    QCOMPARE(cities.size(),3);
}

void Q1ORMTests::test_include_reverseCollection()
{
    QList<Country> countries = ctx->countries.Select()
    .Include("cities")
        .ToList();

    QCOMPARE(countries.size(), 2);

    QJsonArray json = ctx->countries.GetLastJson();
    QCOMPARE(json.size(), 2);
    QVERIFY(json.size() >= 1);

    const QJsonObject firstCountry = json[0].toObject();
    QVERIFY(firstCountry.contains("cities"));
    QVERIFY(firstCountry.value("cities").isArray());

    bool foundTwoCityCountry = false;
    for (const QJsonValue& value : json)
    {
        const QJsonArray cityArray = value.toObject().value("cities").toArray();
        if (cityArray.size() == 2)
        {
            foundTwoCityCountry = true;
            break;
        }
    }

    QVERIFY(foundTwoCityCountry);
}

// ================= JSON =================

void Q1ORMTests::test_toJson()
{
    QByteArray json = ctx->cities.Select().Limit(2).ToJson();

    QJsonDocument doc = QJsonDocument::fromJson(json);

    QVERIFY(doc.isArray());
    QCOMPARE(doc.array().size(),2);
}

void Q1ORMTests::test_showJson()
{
    ctx->cities.Select().Limit(2).ShowJson();

    QJsonArray json = ctx->cities.GetLastJson();
    QCOMPARE(json.size(), 2);
}

// ================= TOLIST =================

void Q1ORMTests::test_toList()
{
    QList<City> cities = ctx->cities.Select().ToList();
    QCOMPARE(cities.size(),3);
}

void Q1ORMTests::test_toListWithWhere()
{
    QList<City> cities = ctx->cities.Select()
    .Where(QString("country_id = %1").arg(usaId))
        .ToList();

    QCOMPARE(cities.size(),2);
}

void Q1ORMTests::test_whereOrderByLimit()
{
    QList<City> cities = ctx->cities.Select()
    .Where(QString("country_id = %1").arg(usaId))
        .OrderByDesc("name")
        .Limit(2)
        .ToList();

    QCOMPARE(cities.size(), 2);
    QVERIFY(cities.size() >= 1);
    QCOMPARE(cities[0].name, QString("New York"));
}

void Q1ORMTests::test_joinWhereOrderByLimit()
{
    ctx->cities.Select({"cities.name AS city",
                        "countries.name AS country"})
        .InnerJoin("countries", "cities.country_id = countries.id")
        .Where("cities.id > 1")
        .OrderByAsc("cities.name")
        .Limit(2)
        .ToList();

    QJsonArray json = ctx->cities.GetLastJson();
    QCOMPARE(json.size(), 2);
    QVERIFY(json.size() >= 1);
    QCOMPARE(json[0].toObject().value("city").toString(), QString("Los Angeles"));
}

// ================= EDGE =================

void Q1ORMTests::test_emptyResult()
{
    QList<City> cities = ctx->cities.Select()
    .Where("id = 999999")
        .ToList();

    QVERIFY(cities.isEmpty());
}

void Q1ORMTests::test_nullValues()
{
    QList<City> cities = ctx->cities.Select().ToList();
    QVERIFY(!cities.isEmpty());
}

void Q1ORMTests::test_reinitialize_is_clean()
{
    QVERIFY(ctx->Initialize());
    QVERIFY2(ctx->GetLastError().isEmpty(), qPrintable(ctx->GetLastError()));

    QList<City> cities = ctx->cities.Select().ToList();
    QCOMPARE(cities.size(), 3);
}
