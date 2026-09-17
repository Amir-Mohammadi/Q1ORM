# Automatic database setup

Q1ORM prepares the database when your application calls `Initialize()`:

```cpp
ApplicationDbContext context(&connection);
if (!context.Initialize()) {
    qCritical() << context.GetLastError();
    return 1;
}
```

It opens the database, creates missing tables, and checks mapped columns,
indexes, and relationships. Existing tables and saved rows are reused.
If you add a supported column to your map, startup adds that missing column.
There are no migration files, history records, or terminal commands to manage.

An unchanged schema is checked but not recreated. Initialization does not insert
application data. TestExample deliberately clears and reseeds its test tables
before each ordinary test; use only a dedicated test database with that runner.

Use the same database path on each run. A relative SQLite filename such as
`q1orm_test2.sqlite` is relative to the process working directory. Starting from
a different directory can therefore open a different file. Use an absolute
database path in your own application if its working directory can change.

Unmapped columns and tables are always preserved. Renames, arbitrary type conversions,
and primary-key changes require explicit schema changes; initialization reports
unsupported changes rather than claiming success. Adding a required column to
a populated table may require a SQL default or backfilling data first.

`Q1Migration` and `Q1MigrationQuery` remain the native catalog and SQL helpers
in `Q1Core/Q1Migration/`. Their source files and class names are retained.
No external framework is used. Old
`__q1_migrations` tables, if present in an existing database, are left untouched
and are no longer read or written.

The maps describe the desired schema; the database catalog describes the current
schema. Each startup compares these directly, so there are no version numbers or
saved snapshots to become stale. Column matching uses a name lookup instead of
comparing every declared column with every database column.

A renamed C++ member can keep its existing database column with
`HasColumnName("original_name")`. Changing the database column name itself is
an explicit operation; setup does not guess which old column to rename.

Schema updates use a transaction on SQLite, PostgreSQL, and SQL Server. MySQL
DDL commits implicitly, so a later failure can leave earlier changes applied;
correct the reported problem and run initialization again.
