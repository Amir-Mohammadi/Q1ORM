#include "SqlGenerationTests.h"

#include <QtTest/QtTest>

#include <Q1Core/Q1Migration/Q1MigrationQuery.h>

void SqlGenerationTests::test_postgresqlTranslatorStillUsesPostgresDialect()
{
    Q1MigrationQuery query(DatabaseType::PostgreSQL);

    QCOMPARE(query.GetDatabasesSQL(), QString("SELECT datname FROM pg_database WHERE datistemplate = false"));
    QVERIFY(query.AddDatabaseSQL("sample_db").contains("ENCODING='UTF8'"));
    QVERIFY(query.DropTableSQL("cities").contains("CASCADE"));
}

void SqlGenerationTests::test_sqlServerTranslatorBuildsIdentityTable()
{
    Q1MigrationQuery query(DatabaseType::SQLServer);

    Q1Table table;
    table.SetName("cities");
    table.columns.append(Q1Column("id", INTEGER, 0, false, true, "GENERATED ALWAYS AS IDENTITY", true));
    table.columns.append(Q1Column("name", VARCHAR, 120, false, false));

    const QString sql = query.AddTableSQL(table);

    QVERIFY(sql.contains("IF OBJECT_ID"));
    QVERIFY(sql.contains("CREATE TABLE [cities]"));
    QVERIFY(sql.contains("[id] INT IDENTITY(1,1) PRIMARY KEY"));
    QVERIFY(sql.contains("[name] NVARCHAR(120) NOT NULL"));
}

void SqlGenerationTests::test_sqlServerTranslatorBuildsDefaultConstraintStatements()
{
    Q1MigrationQuery query(DatabaseType::SQLServer);

    const QString dropDefault = query.DropColumnDefaultSQL("cities", "name");
    const QString setDefault = query.SetColumnDefaultSQL("cities", "name", "active");

    QVERIFY(dropDefault.contains("sys.default_constraints"));
    QVERIFY(setDefault.contains("DROP CONSTRAINT"));
    QVERIFY(setDefault.contains("ADD CONSTRAINT [DF_cities_name] DEFAULT N'active' FOR [name]"));
}

void SqlGenerationTests::test_sqlServerTranslatorUsesMetadataForNullabilityChanges()
{
    Q1MigrationQuery query(DatabaseType::SQLServer);

    const QString setNullable = query.SetColumnNullableSQL("cities", "name");
    const QString setNotNull = query.DropColumnNullableSQL("cities", "name");

    QVERIFY(setNullable.contains("sys.columns"));
    QVERIFY(setNullable.contains("ALTER TABLE [cities] ALTER COLUMN [name]"));
    QVERIFY(setNullable.contains("NULL"));
    QVERIFY(setNotNull.contains("NOT NULL"));
}
