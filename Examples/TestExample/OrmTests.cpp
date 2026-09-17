#include "OrmTests.h"
#include "CityService.h"
#include <Q1Core/Q1Migration/Q1MigrationQuery.h>

#include <QJsonDocument>
#include <QtTest>
#include <barrier>
#include <thread>
#include <vector>

int runModelBuilderTests();
int runSchemaScaleTests();

std::unique_ptr<Q1Connection> OrmTests::makeConnection() const
{
    return std::make_unique<Q1Connection>(
        driver, qEnvironmentVariable("Q1ORM_DB_HOST", "localhost"), database,
        qEnvironmentVariable("Q1ORM_DB_USER"), qEnvironmentVariable("Q1ORM_DB_PASSWORD"),
        qEnvironmentVariableIntValue("Q1ORM_DB_PORT"));
}

void OrmTests::initTestCase()
{
    QVERIFY(directory.isValid());
    const QString backend = qEnvironmentVariable("Q1ORM_TEST_DRIVER", "SQLITE").toUpper();
    if (backend == "SQLITE")
        database = directory.filePath("orm.sqlite");
    else
    {
        QVERIFY2(backend == "POSTGRES" || backend == "MYSQL" || backend == "SQLSERVER",
                 "Q1ORM_TEST_DRIVER must be SQLITE, POSTGRES, MYSQL, or SQLSERVER.");
        driver = backend == "POSTGRES" ? Q1Driver::POSTGRE_SQL
                 : backend == "MYSQL" ? Q1Driver::MYSQL : Q1Driver::SQLSERVER;
        database = qEnvironmentVariable("Q1ORM_DB_NAME");
        QVERIFY2(!database.isEmpty(), "Set Q1ORM_DB_NAME to a dedicated test database.");
    }
    qInfo().noquote() << "Backend:" << backend << "| Available Qt drivers:"
                     << QSqlDatabase::drivers().join(", ");
}

void OrmTests::init()
{
    const QString test = QString::fromLatin1(QTest::currentTestFunction());
    if (test == "modelBuilder" || test == "schemaScale" || test == "sqlGeneration")
        return;
    connection = makeConnection();
    context = std::make_unique<ApplicationContext>(*connection);
    QVERIFY2(context->Initialize(), qPrintable(context->GetLastError()));
    QVERIFY(context->cities.Delete("id > 0"));
    QVERIFY(context->countries.Delete("id > 0"));
    firstCountry = {0, "Canada"};
    secondCountry = {0, "Japan"};
    QVERIFY(context->countries.Insert(firstCountry));
    QVERIFY(context->countries.Insert(secondCountry));
    QList<City> cities = {
        {0, "Toronto", firstCountry.id, 100},
        {0, "Ottawa", firstCountry.id, 200},
        {0, "Tokyo", secondCountry.id, 300}
    };
    QVERIFY2(context->cities.InsertRange(cities), qPrintable(context->cities.GetLastError()));
}

void OrmTests::cleanup()
{
    context.reset();
    connection.reset();
}

void OrmTests::crud()
{
    City city{0, "Montreal", firstCountry.id, 400};
    QVERIFY(context->cities.Insert(city));
    QVERIFY(city.id > 0);
    QCOMPARE(context->cities.Select().Where("id", Q1Operator::Equal, city.id).First().name,
             QString("Montreal"));
    city.name = "Updated";
    city.population = 450;
    QVERIFY(context->cities.UpdateById(city, city.id));
    const auto updated = context->cities.Select().Where("id", Q1Operator::Equal, city.id).ToList();
    QCOMPARE(updated.size(), 1);
    QCOMPARE(updated.first().name, city.name);
    QCOMPARE(updated.first().population, 450);
    QVERIFY(context->cities.DeleteById(city.id));
    QVERIFY(!context->cities.Select().Where("id", Q1Operator::Equal, city.id).Any());
    QCOMPARE(context->cities.Select().Count(), 3);
}

void OrmTests::bulkCrud()
{
    QList<City> cities = {{0, "Bulk One", firstCountry.id, 10},
                          {0, "Bulk Two", secondCountry.id, 20}};
    QVERIFY(context->cities.InsertRange(cities));
    QVERIFY(cities[0].id > 0 && cities[1].id > 0 && cities[0].id != cities[1].id);
    for (auto& city : cities)
        city.population = 50;
    QVERIFY(context->cities.UpdateRange(cities));
    QCOMPARE(context->cities.Select().Where("population", Q1Operator::Equal, 50).Count(), 2);
    QVERIFY(context->cities.DeleteRange(cities));
    QCOMPARE(context->cities.Select().Count(), 3);
}

void OrmTests::selects()
{
    QCOMPARE(context->cities.SelectAll().size(), 3);
    QCOMPARE(context->cities.Select().Where("population", Q1Operator::GreaterThan, 100).Count(), 2);
    QCOMPARE(context->cities.Select().Where("name = 'Tokyo'").OrWhere("name = 'Ottawa'").Count(), 2);
    const auto ordered = context->cities.Select().OrderByAsc("name").Limit(2).ToList();
    QCOMPARE(ordered.size(), 2);
    QCOMPARE(ordered[0].name, QString("Ottawa"));
    QCOMPARE(ordered[1].name, QString("Tokyo"));
    const auto page = context->cities.Select().OrderByDesc("population").Skip(1).Take(1).ToList();
    QCOMPARE(page.size(), 1);
    QCOMPARE(page.first().population, 200);
    QVERIFY(!context->cities.Select().Where("population", Q1Operator::LessThan, 0).Any());
}

void OrmTests::boundValues()
{
    City city{0, QString::fromUtf8("O'Brien 東京 ' OR 1=1 --"), firstCountry.id, 40};
    QVERIFY(context->cities.Insert(city));
    const auto rows = context->cities.Select().Where("name", Q1Operator::Equal, city.name).ToList();
    QCOMPARE(rows.size(), 1);
    QCOMPARE(rows.first().name, city.name);
    QCOMPARE(context->cities.Select().Where("name", Q1Operator::Equal, "' OR 1=1 --").Count(), 0);
    QCOMPARE(context->cities.Select().Count(), 4);
}

void OrmTests::aggregates()
{
    QCOMPARE(context->cities.Select().Count(), 3);
    QCOMPARE(context->cities.Select().Min<int>("population"), 100);
    QCOMPARE(context->cities.Select().Max<int>("population"), 300);
    QCOMPARE(context->cities.Select().Sum<int>("population"), 600);
    QCOMPARE(context->cities.Select().Avg<double>("population"), 200.0);
    QCOMPARE(context->cities.Select().Distinct().Count("country_id"), 2);
    QCOMPARE(context->cities.Select().Where("country_id", Q1Operator::Equal, firstCountry.id)
                 .Sum<int>("population"), 300);
}

void OrmTests::grouping()
{
    context->cities.Select({"country_id", "COUNT(*) AS city_count"})
        .GroupBy("country_id").Having("COUNT(*) > 1").ToList();
    const auto rows = context->cities.GetLastJson();
    QCOMPARE(rows.size(), 1);
    QCOMPARE(rows.first().toObject().value("country_id").toVariant().toInt(), firstCountry.id);
    QCOMPARE(rows.first().toObject().value("city_count").toVariant().toInt(), 2);
}

void OrmTests::joinsAndIncludes()
{
    QCOMPARE(context->cities.Select()
                 .InnerJoin("test_countries", "test_cities.country_id = test_countries.id")
                 .ToList().size(), 3);
    Country empty{0, "Empty"};
    QVERIFY(context->countries.Insert(empty));
    QCOMPARE(context->countries.Select()
                 .LeftJoin("test_cities", "test_countries.id = test_cities.country_id")
                 .ToList().size(), 4);
    if (driver != Q1Driver::SQLITE)
        QCOMPARE(context->countries.Select()
                     .RightJoin("test_cities", "test_countries.id = test_cities.country_id")
                     .ToList().size(), 3);
    if (driver == Q1Driver::POSTGRE_SQL || driver == Q1Driver::SQLSERVER)
        QCOMPARE(context->countries.Select()
                     .FullJoin("test_cities", "test_countries.id = test_cities.country_id")
                     .ToList().size(), 4);
    QCOMPARE(context->cities.Select().Include("test_countries").ToList().size(), 3);
    for (const auto& row : context->cities.GetLastJson())
    {
        const auto object = row.toObject();
        const auto related = object.value("test_countries").toArray();
        QCOMPARE(related.size(), 1);
        const int countryId = object.value("country_id").toVariant().toInt();
        QCOMPARE(related.first().toObject().value("id").toVariant().toInt(), countryId);
        QCOMPARE(related.first().toObject().value("name").toString(),
                 countryId == firstCountry.id ? firstCountry.name : secondCountry.name);
    }
}

void OrmTests::jsonAndRawSql()
{
    const auto document = QJsonDocument::fromJson(
        context->cities.Select().OrderByAsc("population").Limit(2).ToJson());
    QVERIFY(document.isArray());
    QCOMPARE(document.array().size(), 2);
    QCOMPARE(document.array().first().toObject().value("name").toString(), QString("Toronto"));
    QCOMPARE(context->cities.ExecuteScalar("SELECT COUNT(*) FROM test_cities").toInt(), 3);
    const auto rows = context->cities.ExecuteQuery("SELECT name FROM test_cities ORDER BY population");
    QCOMPARE(rows.size(), 3);
    QCOMPARE(rows.first().value("name").toString(), QString("Toronto"));
}

void OrmTests::changeTracking()
{
    City inserted{0, "Tracked insert", firstCountry.id, 50};
    QVERIFY(context->cities.Insert(inserted));
    QVERIFY(context->cities.IsTracked(inserted));
    QVERIFY(!context->cities.IsDirty(inserted));
    context->cities.ClearTracking();
    auto city = context->cities.Select().Where("name", Q1Operator::Equal, "Toronto").First();
    QVERIFY(context->cities.IsTracked(city));
    QVERIFY(!context->cities.IsDirty(city));
    context->cities.Select({"id", "name"}).Where("id", Q1Operator::Equal, city.id).ToList();
    QVERIFY(!context->cities.IsDirty(city));
    city.name = "Changed";
    QCOMPARE(context->cities.GetChangedColumns(city), QStringList{"name"});
    QVERIFY(context->cities.UpdateChangedById(city, city.id));
    QVERIFY(!context->cities.IsDirty(city));
    QCOMPARE(context->cities.Select().Where("id", Q1Operator::Equal, city.id).First().name,
             QString("Changed"));
    QVERIFY(context->cities.UpdateChangedById(city, city.id));
    context->cities.ClearTracking();
    QCOMPARE(context->cities.TrackedCount(), 0);
    QVERIFY(!context->cities.IsTracked(city));
}

void OrmTests::transactions()
{
    {
        auto transaction = connection->Transaction();
        QVERIFY(connection->InTransaction());
        City city{0, "Committed", firstCountry.id, 10};
        QVERIFY(context->cities.Insert(city));
        QVERIFY(transaction.Commit());
    }
    QCOMPARE(context->cities.Select().Count(), 4);
    {
        auto transaction = connection->Transaction();
        City city{0, "Rolled back", firstCountry.id, 10};
        QVERIFY(context->cities.Insert(city));
    }
    QCOMPARE(context->cities.Select().Count(), 4);
    QVERIFY(connection->BeginTransaction());
    QVERIFY(context->cities.Delete("population = 100"));
    QVERIFY(connection->RollbackTransaction());
    QCOMPARE(context->cities.Select().Count(), 4);
    QVERIFY(connection->BeginTransaction());
    QVERIFY(connection->BeginTransaction());
    City nested{0, "Nested rollback", firstCountry.id, 10};
    QVERIFY(context->cities.Insert(nested));
    QVERIFY(connection->RollbackTransaction());
    QVERIFY(!connection->CommitTransaction());
    QCOMPARE(context->cities.Select().Count(), 4);
    QVERIFY(!connection->InTransaction());
}

void OrmTests::constraints()
{
    City orphan{0, "Orphan", -1, 10};
    QVERIFY(!context->cities.Insert(orphan));
    QVERIFY(!context->cities.GetLastError().isEmpty());
    QCOMPARE(context->cities.Select().Count(), 3);
    QVERIFY(context->countries.DeleteById(firstCountry.id));
    QCOMPARE(context->countries.Select().Count(), 1);
    QCOMPARE(context->cities.Select().Count(), 1);
    QCOMPARE(context->cities.Select().First().country_id, secondCountry.id);
}

void OrmTests::connectionLifecycle()
{
    auto local = makeConnection();
    QVERIFY(local->Connect());
    QVERIFY(local->Connect());
    local->Disconnect();
    QVERIFY(local->IsOpen());
    local->Disconnect();
    QVERIFY(!local->IsOpen());
    QVERIFY(local->Connect());
    {
        QSqlQuery query(local->database);
        QVERIFY(query.exec("SELECT COUNT(*) FROM test_cities"));
        QVERIFY(query.next());
        QCOMPARE(query.value(0).toInt(), 3);
    }
    local->Disconnect();
}

void OrmTests::errorHandling()
{
    City city{0, "Unsafe update", firstCountry.id, 10};
    QVERIFY(!context->cities.Update(city, ""));
    QVERIFY(!context->cities.GetLastError().isEmpty());
    QVERIFY(!context->cities.Delete(""));
    QCOMPARE(context->cities.Select().Count(), 3);
    const auto invalid = context->cities.Select().Where("missing_column = 1").ToList();
    QVERIFY(invalid.isEmpty());
    QVERIFY(!context->cities.GetLastError().isEmpty());
    QCOMPARE(context->cities.Select().Count(), 3);
    QList<City> batch = {{0, "Would be inserted", firstCountry.id, 10},
                        {0, "Invalid foreign key", -1, 20}};
    QVERIFY(!context->cities.InsertRange(batch));
    QCOMPARE(context->cities.Select().Count(), 3);
}

void OrmTests::sqlGeneration()
{
    Q1Entity<Country> countries;
    CountryMap::ConfigureEntity(countries);
    Q1MigrationQuery postgres(DatabaseType::PostgreSQL);
    Q1MigrationQuery sqlServer(DatabaseType::SQLServer);
    Q1MigrationQuery mysql(DatabaseType::MySQL);
    Q1MigrationQuery sqlite(DatabaseType::SQLite);
    QVERIFY(postgres.GetDatabasesSQL().contains("pg_database"));
    QVERIFY(postgres.AddTableSQL(*countries.GetTablePtr()).contains("SERIAL"));
    QVERIFY(sqlServer.AddTableSQL(*countries.GetTablePtr()).contains("IDENTITY"));
    QVERIFY(mysql.AddTableSQL(*countries.GetTablePtr()).contains("AUTO_INCREMENT"));
    QVERIFY(sqlite.AddTableSQL(*countries.GetTablePtr()).contains("AUTOINCREMENT"));
}

void OrmTests::threadedCrud()
{
    constexpr int workerCount = 4;
    constexpr int iterations = 10;
    std::barrier start(workerCount);
    std::vector<QString> failures(workerCount);
    std::vector<std::thread> workers;
    for (int workerIndex = 0; workerIndex < workerCount; ++workerIndex)
    {
        workers.emplace_back([&, workerIndex] {
            auto local = makeConnection();
            ApplicationContext localContext(*local);
            Q1ModelBuilder builder(&localContext);
            builder.ApplyMap<CityMap>().ApplyMap<CountryMap>();
            const bool ready = builder.Build() && local->Connect();
            start.arrive_and_wait();
            if (!ready)
            {
                failures[workerIndex] = "Worker connection or mapping failed.";
                return;
            }
            for (int iteration = 0; iteration < iterations; ++iteration)
            {
                City city{0, QString("Worker %1 row %2").arg(workerIndex).arg(iteration),
                          firstCountry.id, iteration};
                if (!localContext.cities.Insert(city))
                {
                    failures[workerIndex] = localContext.cities.GetLastError();
                    return;
                }
                city.population = 999;
                if (!localContext.cities.UpdateById(city, city.id))
                {
                    failures[workerIndex] = localContext.cities.GetLastError();
                    return;
                }
                const auto rows = localContext.cities.Select()
                    .Where("id", Q1Operator::Equal, city.id).ToList();
                if (rows.size() != 1 || rows.first().population != 999 ||
                    rows.first().name != city.name)
                {
                    failures[workerIndex] = "Worker readback mismatch.";
                    return;
                }
                if (!localContext.cities.DeleteById(city.id) ||
                    localContext.cities.Select().Where("id", Q1Operator::Equal, city.id).Any())
                {
                    failures[workerIndex] = "Worker delete failed.";
                    return;
                }
            }
        });
    }
    for (auto& worker : workers)
        worker.join();
    for (const auto& failure : failures)
        QVERIFY2(failure.isEmpty(), qPrintable(failure));
    QCOMPARE(context->cities.Select().Count(), 3);
}

void OrmTests::services()
{
    CityService service(*context);
    City valid{0, "  Montreal  ", firstCountry.id, 100};
    QVERIFY2(service.Create(valid), qPrintable(service.LastError()));
    QCOMPARE(valid.name, QString("Montreal"));
    QVERIFY(valid.id > 0);
    const auto rows = service.InCountry(firstCountry.id);
    QCOMPARE(rows.size(), 3);
    QCOMPARE(rows.first().name, QString("Montreal"));
    City blank{0, "   ", firstCountry.id, 100};
    QVERIFY(!service.Create(blank));
    QVERIFY(!service.LastError().isEmpty());
    City negative{0, "Invalid", firstCountry.id, -1};
    QVERIFY(!service.Create(negative));
    City orphan{0, "Missing country", -1, 1};
    QVERIFY(!service.Create(orphan));
    QCOMPARE(context->cities.Select().Count(), 4);
}

void OrmTests::modelBuilder()
{
    QCOMPARE(runModelBuilderTests(), 0);
}

void OrmTests::schemaScale()
{
    QCOMPARE(runSchemaScaleTests(), 0);
}
