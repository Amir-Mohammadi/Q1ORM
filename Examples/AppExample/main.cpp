
#include "Q1Core/Q1Context/Q1Connection.h"
#include "Q1Core/Q1Context/Q1Context.h"
#include "Q1Core/Q1Entity/Q1Entity.h"

#include <Q1ORM.h>

#include <QByteArray>
#include <QCoreApplication>
#include <QDebug>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QList>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QString>
#include <QStringList>
#include <QTextStream>
#include <QVariant>

#include <cstdio>
#include <cstdlib>
#include <exception>
#include <mutex>
#include <thread>
#include <vector>

namespace
{

void conciseMessageHandler(
    QtMsgType type,
    const QMessageLogContext& context,
    const QString& message)
{
    Q_UNUSED(context);

    // Set Q1ORM_VERBOSE=1 to retain debug/SQL diagnostics.
    if (type == QtDebugMsg)
        return;

    const QByteArray text = message.toLocal8Bit();
    std::fprintf(stderr, "%s\n", text.constData());

    if (type == QtFatalMsg)
        std::abort();
}

struct DbConfig
{
    Q1Driver driver;

    // Environment-variable namespace, e.g. Q1ORM_MYSQL_HOST.
    const char* envTag;

    // Actual connection defaults.
    const char* host;
    const char* user;
    const char* password;
    const char* database;
    int port;
};

struct DbSettings
{
    Q1Driver driver;
    QString host;
    QString database;
    QString user;
    QString password;
    int port = 0;
};

const DbConfig dbs[] = {
     {
        Q1Driver::POSTGRE_SQL,
        "POSTGRES",
        "127.0.0.1",
        "sa",
        "123",
        "q1prm",
        5432
    },

    {
        Q1Driver::MYSQL,
        "MYSQL",
        "127.0.0.1",
        "root",
        "123",
        "testdb",
        3306
    },

    {
        Q1Driver::SQLSERVER,
        "SQLSERVER",
        "127.0.0.1",
        "sa",
        "P@nd1378",
        "q1orm_test",
        1433
    },
};

QString env(const char* name, const QString& fallback)
{
    const QByteArray value = qgetenv(name);
    return value.isEmpty() ? fallback : QString::fromLocal8Bit(value);
}

int envInt(const char* name, int fallback)
{
    bool ok = false;
    const int value = env(name, QString::number(fallback)).toInt(&ok);
    return ok ? value : fallback;
}

bool removeUnexpectedIdentity(
    QSqlDatabase& database,
    const QString& tableName,
    const QString& columnName,
    QString& error)
{
    QSqlQuery metadata(database);
    metadata.prepare(
        "SELECT is_identity, column_default FROM information_schema.columns "
        "WHERE table_schema = current_schema() "
        "AND table_name = :table_name AND column_name = :column_name");
    metadata.bindValue(QStringLiteral(":table_name"), tableName);
    metadata.bindValue(QStringLiteral(":column_name"), columnName);

    if (!metadata.exec())
    {
        error = metadata.lastError().text();
        return false;
    }

    if (!metadata.next())
        return true;

    const bool isIdentity = metadata.value(0).toString() == QStringLiteral("YES");
    const QString defaultValue = metadata.value(1).toString();
    const bool hasSequenceDefault =
        defaultValue.contains(QStringLiteral("nextval("), Qt::CaseInsensitive);
    if (!isIdentity && !hasSequenceDefault)
        return true;

    QSqlQuery repair(database);
    const QString sql = isIdentity
                            ? QStringLiteral(
                                  "ALTER TABLE \"%1\" ALTER COLUMN \"%2\" "
                                  "DROP IDENTITY IF EXISTS")
                                  .arg(tableName, columnName)
                            : QStringLiteral(
                                  "ALTER TABLE \"%1\" ALTER COLUMN \"%2\" "
                                  "DROP DEFAULT")
                                  .arg(tableName, columnName);
    if (!repair.exec(sql))
    {
        error = repair.lastError().text();
        return false;
    }

    qInfo().noquote()
        << QStringLiteral("[POSTGRES] removed unexpected generated value from %1.%2")
               .arg(tableName, columnName);
    return true;
}

DbSettings ResolveSettings(const DbConfig& db)
{
    const QString prefix =
        QStringLiteral("Q1ORM_%1_")
            .arg(QString::fromLatin1(db.envTag).toUpper());

    const auto read = [&](const char* suffix, const QString& fallback)
    {
        const QByteArray key =
            (prefix + QString::fromLatin1(suffix)).toLocal8Bit();

        return env(key.constData(), fallback);
    };

    // SQLite uses its own file setting; server-wide environment overrides
    // should not replace the SQLite filename with "testdb".
    if (db.driver == Q1Driver::SQLITE)
    {
        return {
            db.driver,
            QString(),
            read("DB_NAME", QString::fromLatin1(db.database)),
            QString(),
            QString(),
            0
        };
    }

    const QByteArray portKey =
        (prefix + QStringLiteral("PORT")).toLocal8Bit();

    return {
        db.driver,
        read(
            "HOST",
            env("Q1ORM_DB_HOST", QString::fromLatin1(db.host))),
        read(
            "DB_NAME",
            env("Q1ORM_TEST_DB_NAME", QString::fromLatin1(db.database))),
        read(
            "USER",
            env("Q1ORM_DB_USER", QString::fromLatin1(db.user))),
        read(
            "PASSWORD",
            env("Q1ORM_DB_PASSWORD", QString::fromLatin1(db.password))),
        envInt(portKey.constData(), envInt("Q1ORM_DB_PORT", db.port))
    };
}

struct Country
{
    int id = 0;
    QString name;
};

struct City
{
    int id = 0;
    QString name;
    int country_id = 0;
};

class CountryMap
{
public:
    static void ConfigureEntity(Q1Entity<Country>& entity)
    {
        entity.ToTableName("countries");
        entity.HasKey<&Country::id>().ValueGeneratedOnAdd();
        entity.Property<&Country::name>();
    }
};

class CityMap
{
public:
    static void ConfigureEntity(Q1Entity<City>& entity)
    {
        entity.ToTableName("cities");
        entity.HasKey<&City::id>().ValueGeneratedOnAdd();
        entity.Property<&City::name>().IsRequired();
        entity.Property<&City::country_id>();

        entity.HasOne<Country>()
            .WithMany()
            .HasForeignKey<&City::country_id>()
            .HasPrincipalKey<&Country::id>();
    }
};

class ExampleContext : public Q1Context
{
public:
    explicit ExampleContext(Q1Connection* options)
        : Q1Context()
    {
        SetConnection(options, false);
        RegisterEntity(&countries);
        RegisterEntity(&cities);
    }

    Q1Entity<City> cities;
    Q1Entity<Country> countries;

protected:
    void OnModelCreating(Q1ModelBuilder& builder) override
    {
        builder.ApplyMap<CountryMap>();
        builder.ApplyMap<CityMap>();
    }
};

QString color(const QString& text, const char* code)
{
    return QStringLiteral("\033[%1m%2\033[0m")
        .arg(QString::fromLatin1(code), text);
}

void line(
    const QString& label,
    const QString& status,
    const char* code)
{
    static QTextStream out(stdout);

    out << label.leftJustified(18, ' ')
        << " "
        << color(status, code)
        << Qt::endl;
}

struct Suite
{
    explicit Suite(const DbConfig& config)
        : config(config),
          settings(ResolveSettings(config)),
          conn(
              settings.driver,
              settings.host,
              settings.database,
              settings.user,
              settings.password,
              settings.port),
          ctx(&conn)
    {
    }

    bool init()
    {
        if (config.driver == Q1Driver::SQLITE)
        {
            qInfo().noquote()
                << QStringLiteral("[%1] file=%2")
                       .arg(
                           QString::fromLatin1(config.envTag),
                           settings.database);
        }
        else
        {
            qInfo().noquote()
                << QStringLiteral(
                       "[%1] host=%2 port=%3 database=%4 user=%5")
                       .arg(QString::fromLatin1(config.envTag))
                       .arg(settings.host)
                       .arg(settings.port)
                       .arg(settings.database)
                       .arg(settings.user);
        }

        // Repair existing PostgreSQL example tables when they can be opened.
        // Initialize() handles creating a missing database below.
        if (config.driver == Q1Driver::POSTGRE_SQL && conn.Connect(false))
        {
            QString repairError;
            for (const QString& tableName : {QStringLiteral("countries"), QStringLiteral("cities")})
            {
                if (!removeUnexpectedIdentity(
                        conn.database,
                        tableName,
                        QStringLiteral("name"),
                        repairError))
                {
                    conn.Disconnect();
                    return fail("schema repair", repairError);
                }
            }
            qInfo().noquote() << "[POSTGRES] example schema preflight complete";
            conn.Disconnect();
        }

        if (!ctx.Initialize())
        {
            QString error = ctx.GetLastError();

            if (error.isEmpty())
                error = conn.ErrorMessage();

            return fail("init", error);
        }

        return seed();
    }

    static void showcaseSection(const QString& title)
    {
        qInfo().noquote()
            << QStringLiteral("\n========== %1 ==========").arg(title);
    }

    static void showcaseValue(
        const QString& label,
        const QVariant& value)
    {
        qInfo().noquote()
            << QStringLiteral("  %1 : %2")
                   .arg(label.leftJustified(24, ' '), value.toString());
    }

    template<typename T>
    static void showcaseValue(const QString& label, const T& value)
    {
        showcaseValue(label, QVariant::fromValue(value));
    }

    bool showcase()
    {
        // Show the walkthrough for SQLite by default.
        // Set Q1ORM_SHOWCASE_ALL=1 for every selected database.
        const bool showcaseAll = qgetenv("Q1ORM_SHOWCASE_ALL") == "1";

        if (!showcaseAll && config.driver != Q1Driver::SQLITE)
            return true;

        showcaseSection("MIGRATION + MODEL METADATA");

        Q1Migration migration(conn);

        const QStringList databases = migration.GetDatabases();
        if (!migration.ErrorMessage().isEmpty())
            return fail("showcase migration", migration.ErrorMessage());

        const QStringList tables = migration.GetTables();
        if (!migration.ErrorMessage().isEmpty())
            return fail("showcase migration", migration.ErrorMessage());

        const QList<Q1Column> countryColumns =
            migration.GetColumns("countries");

        if (!migration.ErrorMessage().isEmpty())
            return fail("showcase migration", migration.ErrorMessage());

        qInfo().noquote()
            << QStringLiteral(
                   "[MIGRATION] history table ready | "
                   "%1 table(s) | %2 country column(s)")
                   .arg(tables.size())
                   .arg(countryColumns.size());

        qInfo().noquote()
            << QStringLiteral("[MIGRATION] visible database(s): %1")
                   .arg(databases.join(", "));

        qInfo().noquote()
            << "[MIGRATION] Context.Initialize() synchronized "
               "tables and relations";

        qInfo().noquote() << "[MODEL] countries columns:";

        for (const Q1Column& column : ctx.countries.GetTableColumns())
        {
            qInfo().noquote()
                << QStringLiteral("  - %1 (%2)%3")
                       .arg(
                           column.name,
                           QString::number(static_cast<int>(column.type)),
                           column.primary_key
                               ? QStringLiteral(" [PK]")
                               : QString());
        }

        showcaseSection("CREATE / INSERT / SELECT / UPDATE / DELETE");

        Country demoCountry{0, "Showcase-Country"};

        if (!ctx.countries.Insert(demoCountry))
            return fail("showcase crud", ctx.countries.GetLastError());

        qInfo().noquote()
            << QStringLiteral("Inserted country id=%1")
                   .arg(demoCountry.id);

        demoCountry.name = "Showcase-Country-Updated";

        if (!ctx.countries.UpdateById(demoCountry, demoCountry.id))
            return fail("showcase crud", ctx.countries.GetLastError());

        ctx.countries.Select()
            .Where(QString("id = %1").arg(demoCountry.id))
            .ShowList();

        if (!ctx.countries.DeleteById(demoCountry.id))
            return fail("showcase crud", ctx.countries.GetLastError());

        showcaseSection("BULK CRUD");

        QList<Country> bulk = {
            {0, "Showcase-Bulk-1"},
            {0, "Showcase-Bulk-2"}
        };

        if (!ctx.countries.InsertRange(bulk))
            return fail("showcase bulk", ctx.countries.GetLastError());

        bulk[0].name += "-Updated";
        bulk[1].name += "-Updated";

        if (!ctx.countries.UpdateRange(bulk))
            return fail("showcase bulk", ctx.countries.GetLastError());

        if (!ctx.countries.DeleteRange(bulk))
            return fail("showcase bulk", ctx.countries.GetLastError());

        showcaseSection("QUERY BUILDER");

        ctx.cities.Select()
            .Where(QStringLiteral("name = :city"))
            .Bind(QStringLiteral(":city"), QStringLiteral("New York"))
            .ShowList();

        ctx.cities.Select()
            .Where("name = 'New York'")
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

        ctx.cities.Select({"country_id"})
            .Distinct()
            .ShowList();

        qInfo().noquote()
            << QStringLiteral("[QUERY] SelectAll -> %1 entity row(s)")
                   .arg(ctx.cities.SelectAll().size());

        showcaseSection("AGGREGATES");

        showcaseValue("count", ctx.cities.Select().Count());
        showcaseValue("min id", ctx.cities.Select().Min<int>("id"));
        showcaseValue("max id", ctx.cities.Select().Max<int>("id"));
        showcaseValue("sum id", ctx.cities.Select().Sum<int>("id"));
        showcaseValue("average id", ctx.cities.Select().Avg<double>("id"));

        showcaseValue(
            "distinct countries",
            ctx.cities.Select().Distinct().Count("country_id"));

        showcaseSection("JOINS");

        ctx.cities.Select({
                "cities.name AS city",
                "countries.name AS country"
            })
            .InnerJoin("countries", "cities.country_id = countries.id")
            .OrderByAsc("cities.name")
            .ShowList();

        ctx.cities.Select({
                "cities.name AS city",
                "countries.name AS country"
            })
            .LeftJoin("countries", "cities.country_id = countries.id")
            .ShowList();

        if (config.driver != Q1Driver::SQLITE)
        {
            ctx.countries.Select({
                    "countries.name AS country",
                    "cities.name AS city"
                })
                .RightJoin("cities", "countries.id = cities.country_id")
                .ShowList();
        }
        else
        {
            qInfo().noquote()
                << "[JOIN] RIGHT JOIN skipped by this SQLite demo";
        }

        // Keep the same FULL JOIN backend selection as testQuery().
        if (config.driver == Q1Driver::POSTGRE_SQL ||
            config.driver == Q1Driver::SQLSERVER)
        {
            ctx.cities.Select({
                    "cities.name AS city",
                    "countries.name AS country"
                })
                .FullJoin("countries", "cities.country_id = countries.id")
                .ShowList();
        }
        else
        {
            qInfo().noquote()
                << "[JOIN] FULL JOIN skipped for this backend";
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

        qInfo().noquote()
            << QString::fromUtf8(ctx.cities.Select().Limit(2).ToJson());

        showcaseSection("CHANGE TRACKING");

        const QList<City> trackedRows =
            ctx.cities.Select()
                .Where("name = 'New York'")
                .ToList();

        if (!trackedRows.isEmpty())
        {
            City tracked = trackedRows.first();

            qInfo().noquote()
                << QStringLiteral("tracked=%1, dirty=%2")
                       .arg(ctx.cities.IsTracked(tracked) ? "true" : "false")
                       .arg(ctx.cities.IsDirty(tracked) ? "true" : "false");

            tracked.name = "New York-Tracked";

            qInfo().noquote()
                << "changed columns:"
                << ctx.cities.GetChangedColumns(tracked).join(", ");

            if (!ctx.cities.UpdateChangedById(tracked, tracked.id))
                return fail("showcase tracking", ctx.cities.GetLastError());

            tracked.name = "New York";

            if (!ctx.cities.UpdateById(tracked, tracked.id))
                return fail("showcase tracking", ctx.cities.GetLastError());

            qInfo().noquote()
                << QStringLiteral("tracked entities: %1")
                       .arg(ctx.cities.TrackedCount());
        }

        showcaseSection("LOW-LEVEL QUERY HELPERS");

        const QString quotedCities = conn.QuoteIdentifier("cities");
        const QString quotedId = conn.QuoteIdentifier("id");
        const QString quotedName = conn.QuoteIdentifier("name");

        showcaseValue(
            "ExecuteScalar count",
            ctx.cities.ExecuteScalar(
                QString("SELECT COUNT(*) FROM %1").arg(quotedCities)));

        const QList<QJsonObject> rawRows =
            ctx.cities.ExecuteQuery(
                QString("SELECT %1, %2 FROM %3 ORDER BY %1")
                    .arg(quotedId, quotedName, quotedCities));

        qInfo().noquote()
            << QStringLiteral("ExecuteQuery rows: %1").arg(rawRows.size());

        if (!rawRows.isEmpty())
        {
            qInfo().noquote()
                << QString::fromUtf8(
                       QJsonDocument(rawRows.first())
                           .toJson(QJsonDocument::Compact));
        }

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
            << "BeginTransaction -> CommitTransaction -> "
               "RollbackTransaction: PASS";

        return true;
    }

    bool seed()
    {
        if (!ctx.cities.Delete("id > 0"))
            return fail("seed clear cities", ctx.cities.GetLastError());

        if (!ctx.countries.Delete("id > 0"))
            return fail("seed clear countries", ctx.countries.GetLastError());

        Country usa{0, "USA"};
        Country canada{0, "Canada"};

        if (!ctx.countries.Insert(usa) ||
            !ctx.countries.Insert(canada))
        {
            return fail("seed countries", ctx.countries.GetLastError());
        }

        usaId = usa.id;
        canadaId = canada.id;

        City ny{0, "New York", usa.id};
        City la{0, "Los Angeles", usa.id};
        City toronto{0, "Toronto", canada.id};

        if (!ctx.cities.Insert(ny) ||
            !ctx.cities.Insert(la) ||
            !ctx.cities.Insert(toronto))
        {
            return fail("seed cities", ctx.cities.GetLastError());
        }

        return true;
    }

    bool testSql()
    {
        Q1MigrationQuery pg(DatabaseType::PostgreSQL);
        Q1MigrationQuery ms(DatabaseType::SQLServer);
        Q1MigrationQuery sqlite(DatabaseType::SQLite);

        Q1Table cities = makeCitiesTable();

        return check(
                   pg.GetDatabasesSQL().contains("pg_database"),
                   "sql postgres")
            && check(
                   ms.AddTableSQL(cities).contains("IDENTITY"),
                   "sql sqlserver")
            && check(
                   sqlite.AddTableSQL(cities).contains("AUTOINCREMENT"),
                   "sql sqlite");
    }

    bool testCrud()
    {
        City seattle{0, "Seattle", usaId};

        if (!ctx.cities.Insert(seattle))
            return fail("crud insert", ctx.cities.GetLastError());

        if (!check(seattle.id > 0, "crud generated id"))
            return false;

        seattle.name = "Seattle-Updated";

        if (!ctx.cities.UpdateById(seattle, seattle.id))
            return fail("crud update", ctx.cities.GetLastError());

        const QList<City> rows =
            ctx.cities.Select()
                .Where(QString("id = %1").arg(seattle.id))
                .ToList();

        if (!check(rows.size() == 1, "crud select"))
            return false;

        if (!ctx.cities.DeleteById(seattle.id))
            return fail("crud delete", ctx.cities.GetLastError());

        return check(
            ctx.cities.Select()
                .Where(QString("id = %1").arg(seattle.id))
                .Count() == 0,
            "crud verify");
    }

    bool testBulkCrud()
    {
        QList<Country> countries = {
            Country{0, "Bulk-1"},
            Country{0, "Bulk-2"}
        };

        if (!ctx.countries.InsertRange(countries))
            return fail("bulk insert", ctx.countries.GetLastError());

        if (!check(
                countries[0].id > 0 && countries[1].id > 0,
                "bulk ids"))
        {
            return false;
        }

        countries[0].name = "Bulk-1-updated";
        countries[1].name = "Bulk-2-updated";

        if (!ctx.countries.UpdateRange(countries))
            return fail("bulk update", ctx.countries.GetLastError());

        if (!check(
                ctx.countries.Select()
                    .Where("name LIKE 'Bulk-%-updated'")
                    .Count() == 2,
                "bulk update verify"))
        {
            return false;
        }

        if (!ctx.countries.DeleteRange(countries))
            return fail("bulk delete", ctx.countries.GetLastError());

        return check(
            ctx.countries.Select()
                .Where("name LIKE 'Bulk-%'")
                .Count() == 0,
            "bulk delete verify");
    }

    bool testAggregate()
    {
        return check(
                   ctx.cities.Select().Count() == 3,
                   "agg count")
            && check(
                   ctx.cities.Select().Min<int>("id") > 0,
                   "agg min")
            && check(
                   ctx.cities.Select().Max<int>("id") >= 3,
                   "agg max")
            && check(
                   ctx.cities.Select().Sum<int>("id") > 0,
                   "agg sum")
            && check(
                   ctx.cities.Select().Avg<double>("id") > 0.0,
                   "agg avg")
            && check(
                   ctx.cities.Select().Distinct().Count("country_id") == 2,
                   "agg distinct");
    }

    bool testQuery()
    {
        if (!check(
                ctx.cities.Select()
                    .Where("name = 'New York'")
                    .Count() == 1,
                "query where"))
        {
            return false;
        }

        if (!check(
                ctx.cities.Select()
                    .Where("name = 'New York'")
                    .OrWhere("name = 'Toronto'")
                    .Count() == 2,
                "query or where"))
        {
            return false;
        }

        const QList<City> byName =
            ctx.cities.Select()
                .OrderByAsc("name")
                .Limit(2)
                .ToList();

        if (!check(byName.size() == 2, "query limit"))
            return false;

        const QList<City> page =
            ctx.cities.Select()
                .OrderByAsc("id")
                .Skip(1)
                .Take(1)
                .ToList();

        if (!check(page.size() == 1, "query pagination"))
            return false;

        if (config.driver != Q1Driver::SQLITE)
        {
            if (!check(
                    !ctx.countries.Select()
                         .RightJoin(
                             "cities",
                             "countries.id = cities.country_id")
                         .ToList()
                         .isEmpty(),
                    "query right join"))
            {
                return false;
            }
        }

        if (config.driver == Q1Driver::POSTGRE_SQL ||
            config.driver == Q1Driver::SQLSERVER)
        {
            if (!check(
                    !ctx.cities.Select()
                         .FullJoin(
                             "countries",
                             "cities.country_id = countries.id")
                         .ToList()
                         .isEmpty(),
                    "query full join"))
            {
                return false;
            }
        }

        return check(
            !ctx.cities.Select()
                 .LeftJoin("countries", "cities.country_id = countries.id")
                 .ToList()
                 .isEmpty(),
            "query left join");
    }

    bool testGrouping()
    {
        ctx.cities.Select({"country_id", "COUNT(*) AS city_count"})
            .GroupBy("country_id")
            .Having("COUNT(*) > 0")
            .ToList();

        return check(
            ctx.cities.GetLastJson().size() == 2,
            "group having");
    }

    bool testJsonAndInclude()
    {
        const QByteArray json =
            ctx.cities.Select().Limit(2).ToJson();

        const QJsonDocument document =
            QJsonDocument::fromJson(json);

        if (!check(
                document.isArray() && document.array().size() == 2,
                "json"))
        {
            return false;
        }

        const QList<City> cities =
            ctx.cities.Select()
                .Include("countries")
                .ToList();

        if (!check(cities.size() == 3, "include rows"))
            return false;

        const QJsonArray result = ctx.cities.GetLastJson();

        return check(
            !result.isEmpty() &&
                result.first().toObject().contains("countries"),
            "include countries");
    }

    bool testConnectionLifecycle()
    {
        // Retain the original reference-counted lifecycle test,
        // but bound cleanup so an unexpected implementation cannot hang.
        int disconnectAttempts = 0;

        while (conn.IsOpen() && disconnectAttempts < 1024)
        {
            conn.Disconnect();
            ++disconnectAttempts;
        }

        if (!check(!conn.IsOpen(), "lifecycle initial disconnect"))
            return false;

        if (!conn.Connect())
            return fail("lifecycle connect", conn.ErrorMessage());

        if (!conn.Connect())
            return fail("lifecycle nested connect", conn.ErrorMessage());

        conn.Disconnect();

        if (!check(conn.IsOpen(), "lifecycle nested disconnect"))
            return false;

        conn.Disconnect();

        if (!check(!conn.IsOpen(), "lifecycle disconnect"))
            return false;

        if (!conn.IsSqlite())
        {
            if (!conn.RootConnect())
                return fail("lifecycle root connect", conn.ErrorMessage());

            conn.RootDisconnect();

            if (!check(!conn.IsRootOpen(), "lifecycle root disconnect"))
                return false;
        }

        return true;
    }

    bool testTransactions()
    {
        if (!conn.Connect())
            return fail("tx connect", conn.ErrorMessage());

        if (!conn.BeginTransaction())
        {
            const QString error = conn.ErrorMessage();
            conn.Disconnect();
            return fail("tx begin", error);
        }

        if (!conn.RollbackTransaction())
        {
            const QString error = conn.ErrorMessage();
            conn.Disconnect();
            return fail("tx rollback", error);
        }

        conn.Disconnect();
        return true;
    }

    bool testThreads()
    {
        std::mutex mutex;
        std::vector<QString> failures;
        std::vector<std::thread> workers;
        workers.reserve(4);

        const auto addFailure = [&](const QString& message)
        {
            std::lock_guard<std::mutex> lock(mutex);
            failures.push_back(message);
        };

        try
        {
            for (int i = 0; i < 4; ++i)
            {
                workers.emplace_back([&, i]()
                {
                    try
                    {
                        Q1Connection local(
                            settings.driver,
                            settings.host,
                            settings.database,
                            settings.user,
                            settings.password,
                            settings.port);

                        if (!local.Connect())
                        {
                            addFailure(
                                QStringLiteral(
                                    "worker %1 connect failed: %2")
                                    .arg(i)
                                    .arg(local.ErrorMessage()));
                            return;
                        }

                        // Destroy the query before disconnecting.
                        {
                            QSqlQuery query(local.database);

                            if (!query.exec(
                                    "SELECT COUNT(*) FROM cities"))
                            {
                                addFailure(
                                    QStringLiteral(
                                        "worker %1 query failed: %2")
                                        .arg(i)
                                        .arg(query.lastError().text()));
                            }
                            else if (!query.next())
                            {
                                addFailure(
                                    QStringLiteral(
                                        "worker %1 query returned no row: %2")
                                        .arg(i)
                                        .arg(query.lastError().text()));
                            }
                            else
                            {
                                const int count = query.value(0).toInt();

                                if (count < 3)
                                {
                                    addFailure(
                                        QStringLiteral("worker %1 count %2")
                                            .arg(i)
                                            .arg(count));
                                }
                            }
                        }

                        local.Disconnect();
                    }
                    catch (const std::exception& exception)
                    {
                        addFailure(
                            QStringLiteral("worker %1 exception: %2")
                                .arg(i)
                                .arg(QString::fromLocal8Bit(
                                    exception.what())));
                    }
                    catch (...)
                    {
                        addFailure(
                            QStringLiteral("worker %1 unknown exception")
                                .arg(i));
                    }
                });
            }
        }
        catch (const std::exception& exception)
        {
            addFailure(
                QStringLiteral("thread creation failed: %1")
                    .arg(QString::fromLocal8Bit(exception.what())));
        }

        for (std::thread& worker : workers)
        {
            if (worker.joinable())
                worker.join();
        }

        if (!failures.empty())
        {
            QStringList messages;

            for (const QString& failure : failures)
                messages.append(failure);

            return fail("threads", messages.join("; "));
        }

        return true;
    }

    bool run()
    {
        lastError.clear();

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

    void appendError(const QString& error)
    {
        if (!lastError.isEmpty())
            lastError += '\n';

        lastError += error;
    }

    bool check(bool condition, const QString& label)
    {
        if (!condition)
            appendError(label);

        return condition;
    }

    bool fail(const QString& label, const QString& message)
    {
        appendError(
            label + ": " +
            (message.isEmpty()
                 ? QStringLiteral("No additional error details")
                 : message));

        return false;
    }

    static Q1Table makeCitiesTable()
    {
        Q1Table table;
        table.SetName("cities");

        table.columns.append(
            Q1Column(
                "id",
                INTEGER,
                0,
                false,
                true,
                "GENERATED ALWAYS AS IDENTITY",
                true));

        table.columns.append(
            Q1Column(
                "name",
                VARCHAR,
                120,
                false,
                false));

        return table;
    }

    const DbConfig& config;
    DbSettings settings;
    Q1Connection conn;
    ExampleContext ctx;
    QString lastError;
    int usaId = 0;
    int canadaId = 0;
};

} // namespace

int main(int argc, char* argv[])
{
    QCoreApplication app(argc, argv);

    const QByteArray verbose =
        qgetenv("Q1ORM_VERBOSE").trimmed().toLower();

    if (verbose != "1" && verbose != "true")
        qInstallMessageHandler(conciseMessageHandler);

    QTextStream out(stdout);

    out << "############################################################"
        << Qt::endl;
    out << "Q1ORM cross-database example" << Qt::endl;
    out << "Set Q1ORM_VERBOSE=1 to show SQL/CRUD diagnostics."
        << Qt::endl;
    out << "Set Q1ORM_ONLY=MYSQL to run only MySQL." << Qt::endl;
    out << "------------------------------------------------------------"
        << Qt::endl;

    qInfo().noquote()
        << "Available Qt SQL drivers:"
        << QSqlDatabase::drivers().join(", ");

    // Empty or ALL runs every backend.
    // Accepted individual values: MYSQL, POSTGRES, SQLSERVER, SQLITE.
    const QString only =
        QString::fromLocal8Bit(qgetenv("Q1ORM_ONLY"))
            .trimmed()
            .toUpper();

    int failures = 0;
    int executed = 0;

    for (const DbConfig& config : dbs)
    {
        const QString tag = QString::fromLatin1(config.envTag);

        if (!only.isEmpty() && only != "ALL" && only != tag)
            continue;

        ++executed;
        out << Qt::endl;

        bool ok = false;
        QString error;

        try
        {
            if (config.driver == Q1Driver::SQLSERVER)
            {
                const DbSettings settings = ResolveSettings(config);
                const bool loopback =
                    settings.host.compare(QStringLiteral("localhost"), Qt::CaseInsensitive) == 0 ||
                    settings.host == QStringLiteral("127.0.0.1") ||
                    settings.host == QStringLiteral("::1");
                // Local demo servers commonly use self-signed certificates.
                if (loopback &&
                    !qEnvironmentVariableIsSet("Q1ORM_SQLSERVER_TRUST_SERVER_CERTIFICATE"))
                {
                    qputenv("Q1ORM_SQLSERVER_TRUST_SERVER_CERTIFICATE", "yes");
                }
            }

            Suite suite(config);
            ok = suite.run();
            error = suite.lastError;
        }
        catch (const std::exception& exception)
        {
            error = QStringLiteral("exception: %1")
                        .arg(QString::fromLocal8Bit(exception.what()));
        }
        catch (...)
        {
            error = QStringLiteral("unknown exception");
        }

        line(
            tag,
            ok ? QStringLiteral("PASS") : QStringLiteral("FAIL"),
            ok ? "32" : "31");

        if (!ok)
        {
            out << "  reason: "
                << (error.isEmpty()
                        ? QStringLiteral("No error details available")
                        : error)
                << Qt::endl;

            ++failures;
        }
    }

    if (executed == 0)
    {
        qCritical().noquote()
            << QStringLiteral(
                   "No database selected. Invalid Q1ORM_ONLY='%1'. "
                   "Use ALL, MYSQL, POSTGRES, SQLSERVER, or SQLITE.")
                   .arg(only);

        return EXIT_FAILURE;
    }

    out << Qt::endl;
    out << "Executed: " << executed
        << ", passed: " << (executed - failures)
        << ", failed: " << failures
        << Qt::endl;

    line(
        "Selected databases",
        failures == 0 ? QStringLiteral("PASS") : QStringLiteral("FAIL"),
        failures == 0 ? "32" : "31");

    return failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
