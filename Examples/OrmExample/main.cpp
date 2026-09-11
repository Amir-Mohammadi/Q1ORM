#include <QCoreApplication>
#include <QDebug>
#include <QJsonArray>
#include <QJsonDocument>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QTextStream>

#include <cstdio>
#include <cstdlib>
#include <mutex>
#include <thread>
#include <vector>

#include <Q1Core/Q1Migration/Q1MigrationQuery.h>
#include <Q1ORM.h>

namespace {
void conciseMessageHandler(QtMsgType type, const QMessageLogContext &context,
                           const QString &message) {
  Q_UNUSED(context);

  // The example is intended to show a readable test summary by default.
  // Keep warnings/errors visible while allowing full SQL diagnostics via
  // Q1ORM_VERBOSE=1.
  if (type == QtDebugMsg)
    return;

  const QByteArray text = message.toLocal8Bit();
  std::fprintf(stderr, "%s\n", text.constData());
  if (type == QtFatalMsg)
    std::abort();
}

struct DbConfig {
  Q1Driver driver;
  const char *tag;
  const char *envTag;
  const char *user;
  const char *password;
  const char *database;
  int port;
};

struct DbSettings {
  Q1Driver driver;
  QString host;
  QString database;
  QString user;
  QString password;
  int port = 0;
};

struct Country {
  int id = 0;
  QString name;
};

struct City {
  int id = 0;
  QString name;
  int country_id = 0;
};

class CountryMap {
public:
  static void ConfigureEntity(Q1Entity<Country> &entity) {
    entity.ToTableName("countries");
    entity.Property(entity.id, "id", false, true,
                    "GENERATED ALWAYS AS IDENTITY");
    entity.Property(entity.name, "name", false, false);
  }

  static QList<Q1Relation> CreateRelations(Q1Entity<Country> &entity) {
    return {entity.Relations("countries", "cities", ONE_TO_MANY, "country_id",
                             "id")};
  }
};

class CityMap {
public:
  static void ConfigureEntity(Q1Entity<City> &entity) {
    entity.ToTableName("cities");
    entity.Property(entity.id, "id", false, true,
                    "GENERATED ALWAYS AS IDENTITY");
    entity.Property(entity.name, "name", false, false);
    entity.Property(entity.country_id, "country_id", false, false);
  }

  static QList<Q1Relation> CreateRelations(Q1Entity<City> &entity) {
    return {entity.Relations("cities", "countries", MANY_TO_ONE, "country_id",
                             "id")};
  }
};

class ExampleContext : public Q1Context {
public:
  explicit ExampleContext(Q1Connection *conn) {
    SetConnection(conn, false);
    RegisterEntity(&cities);
    RegisterEntity(&countries);
  }

  Q1Entity<City> cities;
  Q1Entity<Country> countries;

protected:
  void OnModelCreating(Q1ModelBuilder &builder) override {
    builder.ApplyMap<CityMap>();
    builder.ApplyMap<CountryMap>();
  }
};

const DbConfig dbs[] = {
    {Q1Driver::POSTGRE_SQL, "PostgreSQL", "PG", "postgres", "123", "q1orm_test",
     5432},
    {Q1Driver::SQLSERVER, "SQLServer", "SQLSERVER", "sa", "123", "q1orm_test",
     1433},
    {Q1Driver::MYSQL, "MySQL", "MYSQL", "root", "P@nd1378", "q1orm_test2",
     3306},
    {Q1Driver::SQLITE, "SQLite", "SQLITE", "", "", "q1orm_test.sqlite", 0}};

QString env(const char *name, const QString &fallback) {
  const QByteArray value = qgetenv(name);
  return value.isEmpty() ? fallback : QString::fromLocal8Bit(value);
}

int envInt(const char *name, int fallback) {
  bool ok = false;
  const int value = env(name, QString::number(fallback)).toInt(&ok);
  return ok ? value : fallback;
}

DbSettings ResolveSettings(const DbConfig &db) {
  const QString prefix =
      QStringLiteral("Q1ORM_%1_").arg(QString::fromLatin1(db.envTag).toUpper());
  return {db.driver,
          env((prefix + "HOST").toLocal8Bit().constData(),
              env("Q1ORM_DB_HOST", "localhost")),
          env((prefix + "DB_NAME").toLocal8Bit().constData(),
              env("Q1ORM_TEST_DB_NAME", db.database)),
          env((prefix + "USER").toLocal8Bit().constData(),
              env("Q1ORM_DB_USER", db.user)),
          env((prefix + "PASSWORD").toLocal8Bit().constData(),
              env("Q1ORM_DB_PASSWORD", db.password)),
          envInt((prefix + "PORT").toLocal8Bit().constData(),
                 envInt("Q1ORM_DB_PORT", db.port))};
}

QString color(const QString &text, const char *code) {
  return QStringLiteral("\033[%1m%2\033[0m")
      .arg(QString::fromLatin1(code), text);
}

void line(const QString &label, const QString &status, const char *code) {
  static QTextStream out(stdout);
  out << label.leftJustified(18, ' ') << " " << color(status, code) << Qt::endl;
}

struct Suite {
  explicit Suite(const DbConfig &config)
      : config(config), settings(ResolveSettings(config)),
        conn(settings.driver, settings.host, settings.database, settings.user,
             settings.password, settings.port),
        ctx(&conn) {}

  bool init() {
    if (!ctx.Initialize()) {
      fail("init", ctx.GetLastError());
      return false;
    }
    return seed();
  }

  static void showcaseSection(const QString &title) {
    qInfo().noquote()
        << QStringLiteral("\n========== %1 ==========").arg(title);
  }

  static void showcaseValue(const QString &label, const QVariant &value) {
    qInfo().noquote() << QStringLiteral("  %1 : %2")
                             .arg(label.leftJustified(24, ' '),
                                  value.toString());
  }

  template <typename T>
  static void showcaseValue(const QString &label, const T &value) {
    showcaseValue(label, QVariant::fromValue(value));
  }

  bool showcase() {
    // The API is identical for every driver. Show the complete walkthrough
    // once (SQLite is always available), while all drivers still run the
    // complete assertion suite below. Set Q1ORM_SHOWCASE_ALL=1 to repeat
    // the walkthrough for every configured database.
    const bool showcaseAll = qgetenv("Q1ORM_SHOWCASE_ALL") == "1";
    if (!showcaseAll && config.driver != Q1Driver::SQLITE)
      return true;

    showcaseSection("MIGRATION + MODEL METADATA");
    Q1Migration migration(conn);
    if (!migration.EnsureHistoryTable())
      return fail("showcase migration", migration.ErrorMessage());
    const QStringList databases = migration.GetDatabases();
    const QStringList tables = migration.GetTables();
    if (!migration.ErrorMessage().isEmpty())
      return fail("showcase migration", migration.ErrorMessage());
    const QList<Q1Column> countryColumns = migration.GetColumns("countries");
    if (!migration.ErrorMessage().isEmpty())
      return fail("showcase migration", migration.ErrorMessage());
    qInfo().noquote() << QStringLiteral("[MIGRATION] history table ready | %1 "
                                        "table(s) | %2 country column(s)")
                             .arg(tables.size())
                             .arg(countryColumns.size());
    qInfo().noquote() << QStringLiteral("[MIGRATION] visible database(s): %1")
                             .arg(databases.join(", "));
    qInfo().noquote()
        << "[MIGRATION] Context.Initialize() synchronized tables and relations";
    qInfo().noquote() << "[MODEL] countries columns:";
    for (const Q1Column &column : ctx.countries.GetTableColumns())
      qInfo().noquote() << QStringLiteral("  - %1 (%2)%3")
                               .arg(column.name,
                                    QString::number(
                                        static_cast<int>(column.type)),
                                    column.primary_key ? QStringLiteral(" [PK]")
                                                       : QString());

    showcaseSection("CREATE / INSERT / SELECT / UPDATE / DELETE");
    Country demoCountry{0, "Showcase-Country"};
    if (!ctx.countries.Insert(demoCountry))
      return fail("showcase crud", ctx.countries.GetLastError());
    qInfo().noquote()
        << QStringLiteral("Inserted country id=%1").arg(demoCountry.id);

    demoCountry.name = "Showcase-Country-Updated";
    if (!ctx.countries.UpdateById(demoCountry, demoCountry.id))
      return fail("showcase crud", ctx.countries.GetLastError());
    ctx.countries.Select()
        .Where(QString("id = %1").arg(demoCountry.id))
        .ShowList();

    if (!ctx.countries.DeleteById(demoCountry.id))
      return fail("showcase crud", ctx.countries.GetLastError());

    showcaseSection("BULK CRUD");
    QList<Country> bulk = {{0, "Showcase-Bulk-1"}, {0, "Showcase-Bulk-2"}};
    if (!ctx.countries.InsertRange(bulk))
      return fail("showcase bulk", ctx.countries.GetLastError());
    bulk[0].name += "-Updated";
    bulk[1].name += "-Updated";
    if (!ctx.countries.UpdateRange(bulk) || !ctx.countries.DeleteRange(bulk))
      return fail("showcase bulk", ctx.countries.GetLastError());

    showcaseSection("QUERY BUILDER");
    ctx.cities.Select()
        .Where(QStringLiteral("name = :city"))
        .Bind(QStringLiteral(":city"), QStringLiteral("New York"))
        .ShowList();
    ctx.cities.Select()
        .OrWhere("name = 'New York'")
        .OrWhere("name = 'Toronto'")
        .OrderByDesc("name")
        .Take(2)
        .ShowList();
    ctx.cities.Select()
        .SetColumns({"id", "name"})
        .OrderByAsc("id")
        .Skip(1)
        .Limit(1)
        .ShowList();
    ctx.cities.Select({"country_id"}).Distinct().ShowList();
    qInfo().noquote() << QStringLiteral("[QUERY] SelectAll -> %1 entity row(s)")
                             .arg(ctx.cities.SelectAll().size());

    showcaseSection("AGGREGATES");
    showcaseValue("count", ctx.cities.Select().Count());
    showcaseValue("min id", ctx.cities.Select().Min<int>("id"));
    showcaseValue("max id", ctx.cities.Select().Max<int>("id"));
    showcaseValue("sum id", ctx.cities.Select().Sum<int>("id"));
    showcaseValue("average id", ctx.cities.Select().Avg<double>("id"));
    showcaseValue("distinct countries",
                  ctx.cities.Select().Distinct().Count("country_id"));

    showcaseSection("JOINS");
    ctx.cities.Select({"cities.name AS city", "countries.name AS country"})
        .InnerJoin("countries", "cities.country_id = countries.id")
        .OrderByAsc("cities.name")
        .ShowList();
    ctx.cities.Select({"cities.name AS city", "countries.name AS country"})
        .LeftJoin("countries", "cities.country_id = countries.id")
        .ShowList();
    if (config.driver != Q1Driver::SQLITE) {
      ctx.countries.Select({"countries.name AS country", "cities.name AS city"})
          .RightJoin("cities", "countries.id = cities.country_id")
          .ShowList();
      ctx.cities.Select({"cities.name AS city", "countries.name AS country"})
          .FullJoin("countries", "cities.country_id = countries.id")
          .ShowList();
    } else {
      qInfo().noquote() << "[JOIN] RIGHT/FULL OUTER JOIN skipped: SQLite does "
                           "not support them";
    }

    showcaseSection("GROUP BY + HAVING");
    ctx.cities.Select({"country_id", "COUNT(*) AS city_count"})
        .GroupBy("country_id")
        .Having("COUNT(*) > 0")
        .ShowList();

    showcaseSection("JSON + EAGER LOADING");
    ctx.cities.Select().Limit(2).ShowJson();
    ctx.cities.Select().Include("countries").ShowJson();
    ctx.countries.Select().Include("cities").ShowJson();
    qInfo().noquote() << QString::fromUtf8(
        ctx.cities.Select().Limit(2).ToJson());

    showcaseSection("CHANGE TRACKING");
    const QList<City> trackedRows =
        ctx.cities.Select().Where("name = 'New York'").ToList();
    if (!trackedRows.isEmpty()) {
      City tracked = trackedRows.first();
      qInfo().noquote() << QStringLiteral("tracked=%1, dirty=%2")
                               .arg(ctx.cities.IsTracked(tracked) ? "true"
                                                                  : "false")
                               .arg(ctx.cities.IsDirty(tracked) ? "true"
                                                                : "false");
      tracked.name = "New York-Tracked";
      qInfo().noquote() << "changed columns:"
                        << ctx.cities.GetChangedColumns(tracked).join(", ");
      if (!ctx.cities.UpdateChangedById(tracked, tracked.id))
        return fail("showcase tracking", ctx.cities.GetLastError());
      tracked.name = "New York";
      if (!ctx.cities.UpdateById(tracked, tracked.id))
        return fail("showcase tracking", ctx.cities.GetLastError());
      qInfo().noquote() << QStringLiteral("tracked entities: %1")
                               .arg(ctx.cities.TrackedCount());
    }

    showcaseSection("LOW-LEVEL QUERY HELPERS");
    const QString quotedCities = conn.QuoteIdentifier("cities");
    const QString quotedId = conn.QuoteIdentifier("id");
    const QString quotedName = conn.QuoteIdentifier("name");
    showcaseValue("ExecuteScalar count",
                  ctx.cities.ExecuteScalar(
                      QString("SELECT COUNT(*) FROM %1").arg(quotedCities)));
    const QList<QJsonObject> rawRows =
        ctx.cities.ExecuteQuery(QString("SELECT %1, %2 FROM %3 ORDER BY %1")
                                    .arg(quotedId, quotedName, quotedCities));
    qInfo().noquote()
        << QStringLiteral("ExecuteQuery rows: %1").arg(rawRows.size());
    if (!rawRows.isEmpty())
      qInfo().noquote() << QString::fromUtf8(
          QJsonDocument(rawRows.first()).toJson(QJsonDocument::Compact));

    showcaseSection("TRANSACTION LIFECYCLE");
    if (!conn.BeginTransaction())
      return fail("showcase transaction", conn.ErrorMessage());
    if (!conn.CommitTransaction())
      return fail("showcase transaction", conn.ErrorMessage());
    if (!conn.BeginTransaction())
      return fail("showcase transaction", conn.ErrorMessage());
    if (!conn.RollbackTransaction())
      return fail("showcase transaction", conn.ErrorMessage());
    qInfo().noquote()
        << "BeginTransaction -> CommitTransaction -> RollbackTransaction: PASS";

    return true;
  }

  bool seed() {
    if (!ctx.cities.Delete("id > 0"))
      return fail("seed", "could not clear cities");
    if (!ctx.countries.Delete("id > 0"))
      return fail("seed", "could not clear countries");

    Country usa{0, "USA"};
    Country canada{0, "Canada"};
    if (!ctx.countries.Insert(usa) || !ctx.countries.Insert(canada))
      return fail("seed", ctx.countries.GetLastError());

    usaId = usa.id;
    canadaId = canada.id;

    City ny{0, "New York", usa.id};
    City la{0, "Los Angeles", usa.id};
    City toronto{0, "Toronto", canada.id};

    if (!ctx.cities.Insert(ny) || !ctx.cities.Insert(la) ||
        !ctx.cities.Insert(toronto))
      return fail("seed", "could not insert cities");

    return true;
  }

  bool testSql() {
    Q1MigrationQuery pg(DatabaseType::PostgreSQL);
    Q1MigrationQuery ms(DatabaseType::SQLServer);
    Q1MigrationQuery sqlite(DatabaseType::SQLite);
    Q1Table cities = makeCitiesTable();

    return check(pg.GetDatabasesSQL().contains("pg_database"),
                 "sql postgres") &&
           check(ms.AddTableSQL(cities).contains("IDENTITY"),
                 "sql sqlserver") &&
           check(sqlite.AddTableSQL(cities).contains("AUTOINCREMENT"),
                 "sql sqlite");
  }

  bool testCrud() {
    City seattle{0, "Seattle", usaId};
    if (!ctx.cities.Insert(seattle) || seattle.id <= 0)
      return fail("crud", "insert failed");

    seattle.name = "Seattle-Updated";
    if (!ctx.cities.UpdateById(seattle, seattle.id))
      return fail("crud", "update failed");

    const QList<City> rows =
        ctx.cities.Select().Where(QString("id = %1").arg(seattle.id)).ToList();
    if (!check(rows.size() == 1, "crud select"))
      return false;

    if (!ctx.cities.DeleteById(seattle.id))
      return fail("crud", "delete failed");

    return check(
        ctx.cities.Select().Where(QString("id = %1").arg(seattle.id)).Count() ==
            0,
        "crud verify");
  }

  bool testBulkCrud() {
    QList<Country> countries = {Country{0, "Bulk-1"}, Country{0, "Bulk-2"}};

    if (!ctx.countries.InsertRange(countries))
      return fail("bulk insert", ctx.countries.GetLastError());
    if (!check(countries[0].id > 0 && countries[1].id > 0, "bulk ids"))
      return false;

    countries[0].name = "Bulk-1-updated";
    countries[1].name = "Bulk-2-updated";
    if (!ctx.countries.UpdateRange(countries))
      return fail("bulk update", ctx.countries.GetLastError());

    if (!check(ctx.countries.Select()
                       .Where("name LIKE 'Bulk-%-updated'")
                       .Count() == 2,
               "bulk update verify"))
      return false;

    if (!ctx.countries.DeleteRange(countries))
      return fail("bulk delete", ctx.countries.GetLastError());

    return check(ctx.countries.Select().Where("name LIKE 'Bulk-%'").Count() ==
                     0,
                 "bulk delete verify");
  }

  bool testAggregate() {
    return check(ctx.cities.Select().Count() == 3, "agg count") &&
           check(ctx.cities.Select().Min<int>("id") > 0, "agg min") &&
           check(ctx.cities.Select().Max<int>("id") >= 3, "agg max") &&
           check(ctx.cities.Select().Sum<int>("id") > 0, "agg sum") &&
           check(ctx.cities.Select().Avg<double>("id") > 0.0, "agg avg") &&
           check(ctx.cities.Select().Distinct().Count("country_id") == 2,
                 "agg distinct");
  }

  bool testQuery() {
    if (!check(ctx.cities.Select().Where("name = 'New York'").Count() == 1,
               "query where"))
      return false;
    if (!check(ctx.cities.Select()
                       .Where("name = 'New York'")
                       .OrWhere("name = 'Toronto'")
                       .Count() == 2,
               "query or where"))
      return false;

    const QList<City> byName =
        ctx.cities.Select().OrderByAsc("name").Limit(2).ToList();
    if (!check(byName.size() == 2, "query limit"))
      return false;

    const QList<City> page =
        ctx.cities.Select().OrderByAsc("id").Skip(1).Take(1).ToList();
    if (!check(page.size() == 1, "query pagination"))
      return false;

    if (config.driver != Q1Driver::SQLITE) {
      if (!check(!ctx.countries.Select()
                      .RightJoin("cities", "countries.id = cities.country_id")
                      .ToList()
                      .isEmpty(),
                 "query right join"))
        return false;
    }

    if (config.driver == Q1Driver::POSTGRE_SQL ||
        config.driver == Q1Driver::SQLSERVER) {
      if (!check(!ctx.cities.Select()
                      .FullJoin("countries", "cities.country_id = countries.id")
                      .ToList()
                      .isEmpty(),
                 "query full join"))
        return false;
    }

    return check(!ctx.cities.Select()
                      .LeftJoin("countries", "cities.country_id = countries.id")
                      .ToList()
                      .isEmpty(),
                 "query left join");
  }

  bool testGrouping() {
    ctx.cities.Select({"country_id", "COUNT(*) AS city_count"})
        .GroupBy("country_id")
        .Having("COUNT(*) > 0")
        .ToList();

    return check(ctx.cities.GetLastJson().size() == 2, "group having");
  }

  bool testJsonAndInclude() {
    const QByteArray json = ctx.cities.Select().Limit(2).ToJson();
    const QJsonDocument document = QJsonDocument::fromJson(json);
    if (!check(document.isArray() && document.array().size() == 2, "json"))
      return false;

    const QList<City> cities =
        ctx.cities.Select().Include("countries").ToList();
    if (!check(cities.size() == 3, "include rows"))
      return false;

    const QJsonArray result = ctx.cities.GetLastJson();
    return check(!result.isEmpty() &&
                     result.first().toObject().contains("countries"),
                 "include countries");
  }

  bool testConnectionLifecycle() {
    // Context initialization may retain a connection reference.
    while (conn.IsOpen())
      conn.Disconnect();

    if (!check(conn.Connect(), "lifecycle connect"))
      return false;
    if (!check(conn.Connect(), "lifecycle nested connect"))
      return false;

    conn.Disconnect();
    if (!check(conn.IsOpen(), "lifecycle nested disconnect"))
      return false;

    conn.Disconnect();
    if (!check(!conn.IsOpen(), "lifecycle disconnect"))
      return false;

    if (!conn.IsSqlite()) {
      if (!check(conn.RootConnect(), "lifecycle root connect"))
        return false;
      conn.RootDisconnect();
      if (!check(!conn.IsRootOpen(), "lifecycle root disconnect"))
        return false;
    }

    return true;
  }

  bool testTransactions() {
    if (!check(conn.Connect(), "tx connect"))
      return false;
    if (!check(conn.BeginTransaction(), "tx begin"))
      return false;
    if (!check(conn.RollbackTransaction(), "tx rollback"))
      return false;
    conn.Disconnect();
    return true;
  }

  bool testThreads() {
    std::mutex mutex;
    std::vector<QString> failures;
    std::vector<std::thread> workers;

    for (int i = 0; i < 4; ++i) {
      workers.emplace_back([&, i]() {
        Q1Connection local(settings.driver, settings.host, settings.database,
                           settings.user, settings.password, settings.port);
        if (!local.Connect()) {
          std::lock_guard<std::mutex> lock(mutex);
          failures.push_back(QStringLiteral("worker %1 connect failed: %2")
                                 .arg(i)
                                 .arg(local.ErrorMessage()));
          return;
        }

        QSqlQuery query(local.database);
        if (!query.exec("SELECT COUNT(*) FROM cities") || !query.next()) {
          std::lock_guard<std::mutex> lock(mutex);
          failures.push_back(QStringLiteral("worker %1 query failed: %2")
                                 .arg(i)
                                 .arg(query.lastError().text()));
          local.Disconnect();
          return;
        }

        const int count = query.value(0).toInt();
        if (count < 3) {
          std::lock_guard<std::mutex> lock(mutex);
          failures.push_back(
              QStringLiteral("worker %1 count %2").arg(i).arg(count));
        }
        local.Disconnect();
      });
    }

    for (std::thread &worker : workers)
      worker.join();

    return check(failures.empty(),
                 failures.empty() ? "threads" : failures.front());
  }

  bool run() {
    if (!init())
      return false;

    if (!showcase())
      return false;

    bool ok = true;
    ok &= testSql();
    ok &= testCrud();
    ok &= testBulkCrud();
    ok &= testAggregate();
    ok &= testQuery();
    ok &= testGrouping();
    ok &= testJsonAndInclude();
    ok &= testConnectionLifecycle();
    ok &= testTransactions();
    ok &= testThreads();
    return ok;
  }

  bool check(bool condition, const QString &label) {
    if (!condition)
      lastError = label;
    return condition;
  }

  bool fail(const QString &label, const QString &message) {
    lastError = label + ": " + message;
    return false;
  }

  static Q1Table makeCitiesTable() {
    Q1Table table;
    table.SetName("cities");
    table.columns.append(Q1Column("id", INTEGER, 0, false, true,
                                  "GENERATED ALWAYS AS IDENTITY", true));
    table.columns.append(Q1Column("name", VARCHAR, 120, false, false));
    return table;
  }

  const DbConfig &config;
  DbSettings settings;
  Q1Connection conn;
  ExampleContext ctx;
  QString lastError;
  int usaId = 0;
  int canadaId = 0;
};
} // namespace

int main(int argc, char *argv[]) {
  QCoreApplication app(argc, argv);

  const QByteArray verbose = qgetenv("Q1ORM_VERBOSE");
  if (verbose.isEmpty() || (verbose != "1" && verbose.toLower() != "true"))
    qInstallMessageHandler(conciseMessageHandler);

  static QTextStream out(stdout);

  out << "Q1ORM cross-database example" << Qt::endl;
  out << "Set Q1ORM_VERBOSE=1 to show SQL/CRUD diagnostics." << Qt::endl;
  out << "-----------------------------------------------" << Qt::endl;

  int failures = 0;
  for (const DbConfig &config : dbs) {
    out << Qt::endl;
    Suite suite(config);
    const bool ok = suite.run();
    line(QStringLiteral("%1").arg(config.tag), ok ? "PASS" : "FAIL",
         ok ? "32" : "31");
    if (!ok)
      out << "  reason: " << suite.lastError << Qt::endl;
    failures += ok ? 0 : 1;
  }

  line("All databases", failures == 0 ? "PASS" : "FAIL",
       failures == 0 ? "32" : "31");
  return failures == 0 ? 0 : 1;
}
