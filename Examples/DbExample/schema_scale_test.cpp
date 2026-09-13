#include <Q1ORM.h>
#include <QCoreApplication>
#include <QElapsedTimer>
#include <QLoggingCategory>
#include <QTemporaryDir>
#include <QSet>
#include "SchemaScale/ApplicationDbContext.h"
#include "SchemaScale/TestData.h"

namespace {
using namespace SchemaScale;

#define CHECK(condition) do { if (!(condition)) { \
    qCritical() << "Check failed at line" << __LINE__ << #condition; return false; \
} } while (false)

int schemaVersion(Q1Connection& connection) {
    QSqlQuery query(connection.database);
    if (!query.exec("PRAGMA schema_version") || !query.next()) return -1;
    return query.value(0).toInt();
}

bool initialize(ApplicationDbContext& context, const char* label) {
    QElapsedTimer timer;
    timer.start();
    if (!context.Initialize()) {
        qCritical() << label << context.GetLastError();
        return false;
    }
    qInfo().noquote() << label << ':' << timer.nsecsElapsed() / 1000000.0 << "ms";
    return true;
}

bool verify(Q1Connection& connection, bool populated, bool evolved) {
    Q1Migration schema(connection);
    const QStringList actualTables = schema.GetTables();
    CHECK(schema.ErrorMessage().isEmpty());
    int mappedCount = 0;
    for (const auto& table : actualTables)
        if (table.startsWith("scale_")) ++mappedCount;
    CHECK(mappedCount == tableCount);
    CHECK(!actualTables.contains("__q1_migrations"));
    for (int i = 0; i < tableCount; ++i) {
        const QString table = tableName(i);
        CHECK(actualTables.contains(table));
        const auto columns = schema.GetColumns(table);
        CHECK(schema.ErrorMessage().isEmpty());
        CHECK(columns.size() == (evolved ? 4 : 3));
        CHECK(Q1Column::Contains(columns, "id"));
        CHECK(Q1Column::Contains(columns, "name"));
        CHECK(Q1Column::Contains(columns, "parent_id"));
        CHECK(Q1Column::Contains(columns, "revision") == evolved);
        QSqlQuery query(connection.database);
        CHECK(query.exec(QString("PRAGMA index_list('%1')").arg(table)));
        QSet<QString> indexes;
        while (query.next()) indexes.insert(query.value(1).toString());
        CHECK(indexes.contains(QString("IX_%1_name").arg(table)));
        CHECK(indexes.size() == (i == 0 ? 1 : 2));
        if (i > 0) CHECK(indexes.contains(QString("IX_%1_parent_id").arg(table)));
        CHECK(query.exec(QString("PRAGMA foreign_key_list('%1')").arg(table)));
        if (i > 0) {
            CHECK(query.next());
            CHECK(query.value(2).toString() == tableName(0));
            CHECK(query.value(3).toString() == "parent_id");
            CHECK(query.value(4).toString() == "id");
        }
        CHECK(!query.next());
        CHECK(query.exec(QString("SELECT * FROM \"%1\" ORDER BY id").arg(table)));
        int row = 0;
        while (query.next()) {
            ++row;
            CHECK(query.value("id").toInt() == row);
            CHECK(query.value("name").toString() == rowName(i, row));
            CHECK(query.value("parent_id").toInt() == row);
            if (evolved) CHECK(query.value("revision").toInt() == 7);
        }
        CHECK(row == (populated ? rowsPerTable : 0));
    }
    QSqlQuery check(connection.database);
    CHECK(check.exec("PRAGMA foreign_keys"));
    CHECK(check.next());
    CHECK(check.value(0).toInt() == 1);
    CHECK(check.exec("PRAGMA foreign_key_check"));
    CHECK(!check.next());
    CHECK(check.exec("PRAGMA integrity_check"));
    CHECK(check.next());
    CHECK(check.value(0).toString() == "ok");
    return true;
}

bool run() {
    QTemporaryDir directory;
    CHECK(directory.isValid());
    Q1Connection connection(Q1Driver::SQLITE, "","scale.sqlite", "", "", 0);
    CHECK(connection.Connect());
    int originalVersion;
    {
        ApplicationDbContext context(&connection);
        CHECK(initialize(context, "Create 300 tables + 299 relations + 599 indexes"));
        CHECK(verify(connection, false, false));
        originalVersion = schemaVersion(connection);
        CHECK(originalVersion >= 0);
        QElapsedTimer timer;
        timer.start();
        CHECK(connection.BeginTransaction());
        // Avoid 30000 informational CRUD messages in the test report.
        QLoggingCategory::setFilterRules("*.debug=false\n*.info=false");
        CHECK(context.InsertRows());
        CHECK(connection.CommitTransaction());
        QLoggingCategory::setFilterRules("*.debug=false\n*.info=true");
        qInfo() << "Insert 30000 rows:" << timer.nsecsElapsed() / 1000000.0 << "ms";
    }
    // Close/reopen and rebuild context mappings to simulate a later startup.
    connection.Disconnect();
    CHECK(connection.Connect());
    {
        ApplicationDbContext context(&connection);
        CHECK(initialize(context, "Unchanged startup with 30000 rows"));
        CHECK(schemaVersion(connection) == originalVersion);
        CHECK(verify(connection, true, false));
    }
    {
        ApplicationDbContext context(&connection, true);
        CHECK(initialize(context, "Add required revision column with default to all 300 tables"));
        CHECK(schemaVersion(connection) > originalVersion);
        CHECK(verify(connection, true, true));
    }
    const int evolvedVersion = schemaVersion(connection);
    connection.Disconnect();
    CHECK(connection.Connect());
    {
        ApplicationDbContext context(&connection, true);
        CHECK(initialize(context, "Unchanged startup after schema update"));
        CHECK(schemaVersion(connection) == evolvedVersion);
        CHECK(verify(connection, true, true));
    }
    connection.Disconnect();
    qInfo() << "PASS: all 300 tables, 30000 rows, 599 indexes, 299 foreign keys, and 300 added columns verified.";
    return true;
}
} // namespace

int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);
    QLoggingCategory::setFilterRules("*.debug=false");
    return run() ? 0 : 1;
}
