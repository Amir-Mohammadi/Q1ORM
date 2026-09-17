# Q1ORM


<p align="center">
  <strong>A lightweight, type-safe ORM for C++ and Qt</strong>
</p>

<p align="center">
  Define your C++ models, map them to database tables, initialize the schema automatically, and work with your database through a clean C++ API.
</p>

<p align="center">

![C++](https://img.shields.io/badge/C%2B%2B-20-blue)
![Qt](https://img.shields.io/badge/Qt-5%20%7C%206-green)
![License](https://img.shields.io/badge/license-MIT-yellow)
![SQLite](https://img.shields.io/badge/SQLite-supported-blue)
![PostgreSQL](https://img.shields.io/badge/PostgreSQL-supported-blue)
![MySQL](https://img.shields.io/badge/MySQL-supported-blue)
![SQL%20Server](https://img.shields.io/badge/SQL%20Server-supported-blue)

</p>

---

## ✨ Overview

**Q1ORM** is a Qt-based Object-Relational Mapper for C++20.

It provides a simple way to define your database model directly in C++, connect to a database, automatically prepare and reconcile the schema, and perform CRUD and advanced queries without manually writing SQL for common operations.

Q1ORM is designed around a few core concepts:

* `Q1Connection` — database connection
* `Q1Context` — application database context
* `Q1Entity<T>` — typed entity/table interface
* `Q1ModelBuilder` — model configuration
* `Q1Query<T>` — fluent query API
* `Q1Relation` — table relationships
* `Q1Migration` / `Q1MigrationQuery` — native schema and SQL helpers

The normal application flow is:

```text
C++ Model
   ↓
Entity Mapping
   ↓
Q1Context
   ↓
Q1Connection
   ↓
Initialize()
   ↓
Database Schema
   ↓
CRUD / Queries / Transactions
```

---

## 🚀 Features

* C++20 API
* Qt Core + Qt SQL
* SQLite support
* PostgreSQL support
* MySQL support
* SQL Server support
* Automatic database initialization
* Automatic creation of missing tables
* Automatic addition of supported missing columns
* Primary keys
* Generated identity keys
* Required properties
* Custom table names
* Custom column names
* Relationships
* One-to-one relationships
* One-to-many relationships
* Many-to-one relationships
* Fluent queries
* Filtering
* `AND` / `OR` conditions
* Ordering
* Pagination
* `Skip` / `Take`
* Joins
* Left joins
* Inner joins
* Grouping
* `HAVING`
* Aggregates
* Eager loading / `Include`
* JSON output
* Raw SQL support
* Insert / Update / Delete
* Bulk insert / update / delete
* Change tracking
* Transactions
* Nested transaction support
* Automatic rollback through transaction guards
* SQL logging
* Cross-platform design
* No external ORM framework dependency

---

# 🗄️ Supported Databases

| Database   | Q1 Driver               | Qt SQL Plugin | Default Port |
| ---------- | ----------------------- | ------------- | -----------: |
| SQLite     | `Q1Driver::SQLITE`      | `QSQLITE`     |            — |
| PostgreSQL | `Q1Driver::POSTGRE_SQL` | `QPSQL`       |       `5432` |
| MySQL      | `Q1Driver::MYSQL`       | `QMYSQL`      |       `3306` |
| SQL Server | `Q1Driver::SQLSERVER`   | `QODBC`       |       `1433` |

SQLite does not require a database server.

PostgreSQL, MySQL, and SQL Server require:

* A running database server
* An existing database
* A database user
* Appropriate permissions
* The matching Qt SQL plugin
* Required native client libraries

The application API remains the same across the supported database engines.

---

# 📦 Requirements

* C++20 compatible compiler
* CMake 3.14+
* Qt 5 or Qt 6
* Qt `Core`
* Qt `Sql`

For server databases, install the corresponding Qt SQL plugin.

For SQL Server, an appropriate ODBC driver is also required.

---

# 🔨 Building Q1ORM

Clone the repository:

```bash
git clone https://github.com/Amir-Mohammadi/Q1ORM.git
cd Q1ORM
```

Configure:

```bash
cmake -S . -B build
```

Build:

```bash
cmake --build build
```

If CMake cannot find Qt, specify the Qt installation:

```bash
cmake -S . -B build \
  -DCMAKE_PREFIX_PATH="C:/Qt/6.8.3/msvc2022_64"
```

Then:

```bash
cmake --build build
```

Install the library:

```bash
cmake --install build
```

The release layout is:

```text
Releases/
└── Release-0.1/
    ├── bin/
    ├── include/
    ├── lib/
    └── scripts/
```

---

# 🧩 Using Q1ORM

The basic workflow is:

1. Define your C++ model.
2. Create an entity map.
3. Create a `Q1Context`.
4. Create a `Q1Connection`.
5. Register the entities.
6. Configure the model.
7. Call `Initialize()`.
8. Perform CRUD and queries.

---

# 1. Define a Model

A model can be a simple C++ structure.

```cpp
struct Country
{
    int id = 0;
    QString name;
};
```

No base class is required.

---

# 2. Map the Model

Create a map for the model:

```cpp
struct CountryMap
{
    using EntityType = Country;

    static void ConfigureEntity(Q1Entity<Country>& entity)
    {
        entity.ToTableName("countries");

        entity.HasKey<&Country::id>()
            .ValueGeneratedOnAdd();

        entity.Property<&Country::name>()
            .IsRequired();
    }
};
```

This mapping describes the desired database schema.

---

# 3. Create a Database Context

```cpp
class ApplicationDbContext : public Q1Context
{
public:
    explicit ApplicationDbContext(Q1Connection& connection)
    {
        SetConnection(&connection, false);
        RegisterEntity(&countries);
    }

    Q1Entity<Country> countries;

protected:
    void OnModelCreating(Q1ModelBuilder& builder) override
    {
        builder.ApplyMap<CountryMap>();
    }
};
```

The context owns the entity sets used by your application.

---

# 4. Connect to Database

SQLite is the simplest way to start.

```cpp
Q1Connection connection(
    Q1Driver::SQLITE,
    "",
    "application.sqlite",
    "",
    "",
    0
);
```

or you can connect other databases such as (sqlserver, postgresql and mysql)

```cpp

Q1Connection connection(
    Q1Driver::SqlServer,
   "SQLServer",
   "SQLSERVER",
   "sa",
   "123",
   "q1orm_test",
    1433
);

```

Then create your context:

```cpp
ApplicationDbContext context(connection);
```

---

# 5. Initialize the Database

Call `Initialize()` when your application starts:

```cpp

if (!context.Initialize())
{
    qCritical() << context.GetLastError();
    return 1;
}

context.Initialize();
```

Q1ORM will inspect the database and reconcile supported schema differences.

---

# 🏗️ Automatic Database Setup

One of Q1ORM's main features is automatic schema initialization.

When your application calls:

```cpp
context.Initialize();
```

Q1ORM:

* Opens the configured database
* Checks the current database catalog
* Creates missing tables
* Creates missing columns
* Checks mapped columns
* Checks indexes
* Checks relationships
* Reconciles supported schema changes

Existing tables and existing data are preserved.

There is no requirement to create migration files or maintain migration version numbers for the automatic initialization system.

### Example

Suppose the application initially contains:

```cpp
struct Country
{
    int id = 0;
    QString name;
};
```

and later you add:

```cpp
struct Country
{
    int id = 0;
    QString name;
    QString code;
};
```

and map the new property:

```cpp
entity.Property<&Country::code>();
```

On the next:

```cpp
context.Initialize();
```

Q1ORM can add the missing supported column to the existing table.

---

# 🔄 Schema Reconciliation

Q1ORM compares:

```text
C++ Model Mapping
       │
       ▼
Desired Schema
       │
       │ compare
       ▼
Database Catalog
       │
       ▼
Required Schema Changes
```

It does **not** rely on:

* Migration version numbers
* Saved schema snapshots
* Migration history tables

Column matching is performed using column names.

An unchanged schema is checked but is not unnecessarily recreated.

---

# 🛡️ Existing Database Data

Initialization does not insert application data.

Existing rows are preserved.

For example:

```text
countries
--------------------------------
id | name
--------------------------------
1  | Canada
2  | Germany
3  | Japan
```

Calling:

```cpp
context.Initialize();
```

does not delete or recreate those rows.

---

# ⚠️ Schema Changes That Require Explicit Handling

Automatic initialization should not be treated as a general-purpose migration engine.

The following changes require explicit consideration:

* Renaming a database column
* Arbitrary type conversions
* Primary-key changes
* Destructive schema changes
* Complex data transformations
* Adding required columns to populated tables without a usable default

For example, this:

```cpp
entity.Property<&Country::name>()
    .HasColumnName("country_name");
```

can preserve an existing database column named `country_name`.

Q1ORM does not guess that:

```text
name → country_name
```

means the old database column should be renamed.

If the database itself must be renamed, perform that operation explicitly.

---

# 🧭 Custom Column Names

A C++ member does not have to use the same name as its database column.

```cpp
struct Country
{
    int id = 0;
    QString name;
};
```

Mapping:

```cpp
entity.Property<&Country::name>()
    .HasColumnName("country_name");
```

The database will contain:

```text
country_name
```

while your C++ code continues to use:

```cpp
country.name
```

---

# 🧱 Tables and Columns

Change the table name:

```cpp
entity.ToTableName("countries");
```

Configure a primary key:

```cpp
entity.HasKey<&Country::id>()
    .ValueGeneratedOnAdd();
```

Configure a required property:

```cpp
entity.Property<&Country::name>()
    .IsRequired();
```

Configure a custom database column name:

```cpp
entity.Property<&Country::name>()
    .HasColumnName("country_name");
```

---

# 🔗 Relationships

Q1ORM supports relationships between entities.

Example:

```text
Country
   │
   └── City
```

A country can have many cities.

```cpp
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
```

Country relationship:

```cpp
static QList<Q1Relation> CreateRelations(Q1Entity<Country>& entity)
{
    QList<Q1Relation> relations;

    relations.append(
        entity.Relations(
            "countries",
            "cities",
            ONE_TO_MANY,
            "country_id",
            "id"
        )
    );

    return relations;
}
```

City relationship:

```cpp
static QList<Q1Relation> CreateRelations(Q1Entity<City>& entity)
{
    QList<Q1Relation> relations;

    relations.append(
        entity.Relations(
            "cities",
            "countries",
            MANY_TO_ONE,
            "country_id",
            "id"
        )
    );

    return relations;
}
```

---

# ✏️ Insert

Insert a new object:

```cpp
Country country;

country.name = "Canada";

if (!context.countries.Insert(country))
{
    qCritical() << "Insert failed";
}
```

For generated primary keys, Q1ORM updates the object with the generated ID.

```cpp
qDebug() << "Generated ID:" << country.id;
```

---

# 📚 Insert Multiple Records

For bulk operations, use:

```cpp
context.countries.InsertRange(countries);
```

Always check the result before continuing with dependent operations.

---

# 🔎 Select

Start a query with:

```cpp
auto query = context.countries.Select();
```

Retrieve rows:

```cpp
const auto countries = context.countries
    .Select()
    .ToList();
```

---

# 🎯 Where

Filter records using bound values:

```cpp
const auto countries = context.countries
    .Select()
    .Where(
        "name",
        Q1Operator::Equal,
        QStringLiteral("Canada")
    )
    .ToList();
```

Bound values should be used for user-provided data.

Do **not** concatenate user input into SQL.

---

# 🔀 Multiple Conditions

Queries can be composed with multiple conditions.

For example:

```cpp
auto countries = context.countries
    .Select()
    .Where("id", Q1Operator::GreaterThan, 10)
    .AndWhere("name", Q1Operator::Equal, "Canada")
    .ToList();
```

OR conditions can also be used:

```cpp
auto countries = context.countries
    .Select()
    .Where("name", Q1Operator::Equal, "Canada")
    .OrWhere("name", Q1Operator::Equal, "Germany")
    .ToList();
```

---

# ↕️ Ordering

Ascending:

```cpp
auto countries = context.countries
    .Select()
    .OrderBy("name", Q1Sort::Ascending)
    .ToList();
```

Descending:

```cpp
auto countries = context.countries
    .Select()
    .OrderBy("id", Q1Sort::Descending)
    .ToList();
```

---

# 📄 Pagination

Use `Skip()` and `Take()`:

```cpp
const auto countries = context.countries
    .Select()
    .OrderBy("id", Q1Sort::Ascending)
    .Skip(20)
    .Take(10)
    .ToList();
```

This represents:

```text
Page 3
-----------
Skip 20
Take 10
```

---

# 🔢 Count

```cpp
const auto count =
    context.countries
        .Select()
        .Count();
```

---

# ✏️ Update

Update an entity by its primary key:

```cpp
country.name = "Canada";

if (!context.countries.UpdateById(country, country.id))
{
    qCritical() << "Update failed";
}
```

---

# 🗑️ Delete

Delete by primary key:

```cpp
if (!context.countries.DeleteById(country.id))
{
    qCritical() << "Delete failed";
}
```

---

# 📦 Bulk Update and Delete

Q1ORM provides bulk operations such as:

```cpp
InsertRange(...)
UpdateRange(...)
DeleteRange(...)
```

Use bulk operations when working with multiple entities rather than repeatedly issuing individual operations.

---

# 🔍 Joins

Q1ORM supports joins through the query API.

Conceptually:

```text
countries
    │
    └── cities
          │
          └── ...
```

Joins can be combined with:

* Filtering
* Ordering
* Grouping
* Aggregates
* Pagination

For complex queries, use trusted SQL expressions and identifiers.

---

# 📥 Eager Loading

Relationships can be loaded together with the main query using `Include`.

Example concept:

```cpp
context.countries
    .Select()
    .Include("cities")
    .ToList();
```

This allows related records to be loaded as part of the query rather than manually querying every relationship.

---

# 📊 Grouping

Group query results:

```cpp
context.countries
    .Select()
    .GroupBy("name");
```

Grouping can be combined with aggregates and `HAVING`.

---

# 🔢 Aggregates

Q1ORM supports aggregate operations including:

* `Count`
* `Min`
* `Max`
* `Sum`
* `Average`
* Distinct aggregates
* Filtered aggregates

Example:

```cpp
const auto count =
    context.countries
        .Select()
        .Count();
```

---

# 🧾 JSON Output

Query results can be converted to JSON for debugging or application-level processing.

```cpp
const auto json =
    context.countries
        .Select()
        .ToJson();
```

---

# 🧮 Raw SQL

Q1ORM also supports raw SQL for operations that do not fit the query builder.

Raw SQL should only contain trusted SQL syntax.

User-provided values should remain bound parameters.

> Binding protects values, not SQL identifiers, SQL clauses, join expressions, or aggregate expressions.

If an identifier such as a sort column comes from a user request, map it through an explicit allowlist before using it.

---

# 🔄 Transactions

Transactions are managed through a transaction guard.

```cpp
auto transaction = connection.Transaction();

if (!connection.InTransaction())
{
    qCritical() << "Could not start transaction";
    return 1;
}

Country country;
country.name = "Transactional";

if (!context.countries.Insert(country))
{
    return 1;
}

if (!transaction.Commit())
{
    qCritical() << "Commit failed";
    return 1;
}
```

If the transaction guard leaves scope without a successful commit, the uncommitted work is rolled back.

This also makes early returns safer.

---

# 🔁 Nested Transactions

Nested transactions join the outer transaction.

They should therefore not be treated as completely independent database transactions.

Keep transactions short and always check:

```cpp
connection.InTransaction()
```

and:

```cpp
transaction.Commit()
```

---

# 🧵 Threading

Qt SQL connections should remain associated with the thread that owns them.

For worker threads:

```text
Worker Thread
    │
    ├── Q1Connection
    │
    ├── Q1Context
    │
    └── Q1Entity
```

Create and destroy the connection and context inside the same worker thread.

Do not share:

* `QSqlDatabase` handles
* `Q1Connection`
* `Q1Context`
* Entity sets

between worker threads.

Initialize the schema before starting worker threads.

Multiple independent connections do not eliminate database-level locking. SQLite, in particular, can experience contention when multiple writers operate concurrently.

---

# 🗄️ Database Connections

## SQLite

```cpp
Q1Connection connection(
    Q1Driver::SQLITE,
    "",
    "application.sqlite",
    "",
    "",
    0
);
```

SQLite does not use a host, username, password, or port.

### SQLite Path

A relative path such as:

```text
application.sqlite
```

is resolved relative to the process working directory.

Therefore:

```text
C:\App\MyProgram.exe
```

and:

```text
C:\App\bin\MyProgram.exe
```

may open different database files if their working directories differ.

For deployed applications, prefer an absolute path or a known writable application-data directory.

---

## PostgreSQL

```cpp
Q1Connection connection(
    Q1Driver::POSTGRE_SQL,
    "localhost",
    "application",
    qEnvironmentVariable("DB_USER"),
    qEnvironmentVariable("DB_PASSWORD"),
    5432
);
```

Qt requires the PostgreSQL SQL plugin:

```text
QPSQL
```

---

## MySQL

```cpp
Q1Connection connection(
    Q1Driver::MYSQL,
    "localhost",
    "application",
    qEnvironmentVariable("DB_USER"),
    qEnvironmentVariable("DB_PASSWORD"),
    3306
);
```

Qt requires:

```text
QMYSQL
```

The Qt MySQL plugin and its native client libraries must match the Qt/compiler environment.

---

## SQL Server

```cpp
Q1Connection connection(
    Q1Driver::SQLSERVER,
    "localhost",
    "application",
    qEnvironmentVariable("DB_USER"),
    qEnvironmentVariable("DB_PASSWORD"),
    1433
);
```

Qt uses:

```text
QODBC
```

The SQL Server ODBC driver must also be installed.

`Q1ORM_SQLSERVER_ODBC_DRIVER` can be used to select the installed SQL Server ODBC driver.

The host argument can also contain a DSN or ODBC-style connection string.

---

# 🧱 Complete Minimal Example

```cpp
#include <QCoreApplication>
#include <QDebug>
#include <QString>

#include <Q1ORM.h>

struct Country
{
    int id = 0;
    QString name;
};

struct CountryMap
{
    using EntityType = Country;

    static void ConfigureEntity(Q1Entity<Country>& entity)
    {
        entity.ToTableName("countries");

        entity.HasKey<&Country::id>()
            .ValueGeneratedOnAdd();

        entity.Property<&Country::name>()
            .IsRequired();
    }
};

class ApplicationDbContext : public Q1Context
{
public:
    explicit ApplicationDbContext(Q1Connection& connection)
    {
        SetConnection(&connection, false);
        RegisterEntity(&countries);
    }

    Q1Entity<Country> countries;

protected:
    void OnModelCreating(Q1ModelBuilder& builder) override
    {
        builder.ApplyMap<CountryMap>();
    }
};

int main(int argc, char* argv[])
{
    QCoreApplication application(argc, argv);

    Q1Connection connection(
        Q1Driver::SQLITE,
        "",
        "application.sqlite",
        "",
        "",
        0
    );

    ApplicationDbContext context(connection);

    if (!context.Initialize())
    {
        qCritical()
            << "Database initialization failed:"
            << context.GetLastError();

        return 1;
    }

    Country country;
    country.name = "Canada";

    if (!context.countries.Insert(country))
    {
        qCritical() << "Insert failed";
        return 1;
    }

    qInfo() << "Generated ID:" << country.id;

    const auto countries = context.countries
        .Select()
        .Where(
            "name",
            Q1Operator::Equal,
            QStringLiteral("Canada")
        )
        .OrderBy(
            "id",
            Q1Sort::Ascending
        )
        .Skip(0)
        .Take(10)
        .ToList();

    for (const auto& item : countries)
    {
        qInfo() << item.id << item.name;
    }

    country.name = "Canada - Updated";

    if (!context.countries.UpdateById(country, country.id))
    {
        qCritical() << "Update failed";
        return 1;
    }

    if (!context.countries.DeleteById(country.id))
    {
        qCritical() << "Delete failed";
        return 1;
    }

    return 0;
}
```

---

# 🧩 CMake Integration

After building Q1ORM, copy:

```text
Releases/Release-0.1/
```

into your application:

```text
MyApplication/
├── CMakeLists.txt
├── main.cpp
└── external/
    └── Q1ORM/
        └── Release-0.1/
            ├── bin/
            ├── include/
            ├── lib/
            └── scripts/
```

Example CMake configuration:

```cmake
cmake_minimum_required(VERSION 3.14)

project(MyApplication LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Core Sql)

set(Q1ORM_RELEASE
    "${CMAKE_CURRENT_SOURCE_DIR}/external/Q1ORM/Release-0.1"
)

add_library(Q1ORM::Q1ORM SHARED IMPORTED)

set_target_properties(Q1ORM::Q1ORM PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES
        "${Q1ORM_RELEASE}/include"
    INTERFACE_COMPILE_FEATURES
        cxx_std_20
)

if(WIN32)

    set_target_properties(Q1ORM::Q1ORM PROPERTIES
        IMPORTED_LOCATION
            "${Q1ORM_RELEASE}/bin/Q1ORM.dll"
        IMPORTED_IMPLIB
            "${Q1ORM_RELEASE}/lib/Q1ORM.lib"
    )

elseif(APPLE)

    set_target_properties(Q1ORM::Q1ORM PROPERTIES
        IMPORTED_LOCATION
            "${Q1ORM_RELEASE}/lib/libQ1ORM.dylib"
    )

else()

    set_target_properties(Q1ORM::Q1ORM PROPERTIES
        IMPORTED_LOCATION
            "${Q1ORM_RELEASE}/lib/libQ1ORM.so"
    )

endif()

add_executable(MyApplication
    main.cpp
)

target_link_libraries(MyApplication
    PRIVATE
        Q1ORM::Q1ORM
        Qt6::Core
        Qt6::Sql
)

if(WIN32)

    add_custom_command(
        TARGET MyApplication
        POST_BUILD

        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "$<TARGET_FILE:Q1ORM::Q1ORM>"
            "$<TARGET_FILE_DIR:MyApplication>"
    )

endif()
```

---

# 📁 Project Structure

```text
Q1ORM/
├── src/
│   ├── Q1Core/
│   ├── Q1Entity/
│   ├── Q1Migration/
│   ├── Q1Query/
│   └── ...
│
├── Examples/
│   ├── SoloExample/
│   ├── DatabaseInstallExample/
│   ├── UnitTestExample/
│   └── ...
│
├── Docs/
│
├── Releases/
│   └── Release-0.1/
│       ├── bin/
│       ├── include/
│       ├── lib/
│       └── scripts/
│
├── Tools/
│
├── CMakeLists.txt
├── LICENSE
└── README.md
```

---

# 🧠 Q1Migration and Q1MigrationQuery

Q1ORM still contains:

```text
Q1Core/Q1Migration/
```

with:

```text
Q1Migration
Q1MigrationQuery
```

These classes remain part of the native Q1ORM implementation and provide schema/catalog and database-specific SQL functionality.

Q1ORM does not depend on an external migration framework.

Older:

```text
__q1_migrations
```

tables, if they already exist in a database, are left untouched.

Q1ORM does not use them for the automatic schema initialization process.

---

# 🔐 Schema Safety

Automatic initialization is intentionally conservative.

### Preserved

Q1ORM preserves:

* Existing tables
* Existing rows
* Unmapped columns
* Unmapped tables
* Existing database objects that are outside the mapped model

### Automatically handled

Supported schema changes can include:

* Missing tables
* Missing mapped columns
* Required indexes
* Supported relationships

### Explicit operations

You should explicitly handle:

* Column renames
* Arbitrary type conversions
* Primary-key changes
* Destructive changes
* Data transformations
* Complex backfills

Before changing a production schema:

1. Back up the database.
2. Test the change against representative data.
3. Verify required columns and defaults.
4. Check relationships and indexes.
5. Run `Initialize()`.
6. Check `GetLastError()`.
7. Validate the resulting schema and data.

A new required column on a populated table may require a SQL default or an explicit data backfill.

---

# 🔄 Transaction Behavior During Schema Updates

Schema updates use transactions on:

* SQLite
* PostgreSQL
* SQL Server

MySQL DDL operations may commit implicitly.

Therefore, if a later MySQL schema operation fails, earlier schema changes may already have been committed.

After correcting the reported problem, run initialization again.

Always test schema changes against the same database engine used in production.

---

# 🧪 Testing

Build the project:

```bash
cmake -S . -B build
cmake --build build
```

Run CTest:

```bash
ctest --test-dir build --output-on-failure
```

On Windows Release builds:

```powershell
ctest --test-dir build -C Release --output-on-failure
```

You can also run the test executable directly.

```text
TestExample
```

List available tests:

```bash
TestExample -functions
```

Run selected tests:

```bash
TestExample crud aggregates
```

Generate JUnit output:

```bash
TestExample -o results.xml,junitxml
```

The test suite covers areas including:

* CRUD
* Bulk CRUD
* Filtering
* Ordering
* Pagination
* Aggregates
* Grouping
* Joins
* Includes
* JSON
* Raw SQL
* Change tracking
* Transactions
* Constraints
* Error handling
* SQL generation
* Connection lifecycle
* Model building
* Schema reconciliation

SQLite is used as the default test backend.

---

# 🧪 Testing With PostgreSQL / MySQL / SQL Server

Use a dedicated test database.

**Do not run the test suite against a production database.**

Example PostgreSQL configuration:

```bash
export Q1ORM_TEST_DRIVER=POSTGRES
export Q1ORM_DB_HOST=localhost
export Q1ORM_DB_NAME=q1orm_test
export Q1ORM_DB_USER=q1orm_test
export Q1ORM_DB_PASSWORD='<password>'
export Q1ORM_DB_PORT=5432

ctest --test-dir build --output-on-failure
```

Windows PowerShell:

```powershell
$env:Q1ORM_TEST_DRIVER = "POSTGRES"
$env:Q1ORM_DB_HOST = "localhost"
$env:Q1ORM_DB_NAME = "q1orm_test"
$env:Q1ORM_DB_USER = "q1orm_test"
$env:Q1ORM_DB_PASSWORD = "<password>"
$env:Q1ORM_DB_PORT = "5432"

ctest --test-dir build -C Release --output-on-failure
```

Supported test selections:

```text
SQLITE
POSTGRES
MYSQL
SQLSERVER
```

---

# 🐞 Debugging

## SQL Logging

Enable SQL logging:

### Linux / macOS

```bash
Q1ORM_VERBOSE=1 ./MyApplication
```

### Windows PowerShell

```powershell
$env:Q1ORM_VERBOSE = "1"
.\MyApplication.exe
```

Do not share logs containing:

* Passwords
* Connection strings
* Personal information
* Confidential application data

---

# 🔌 Check Qt SQL Drivers

If a database driver is not loading:

```cpp
#include <QSqlDatabase>
#include <QDebug>

qInfo() << QSqlDatabase::drivers();
```

Expected drivers may include:

```text
QSQLITE
QPSQL
QMYSQL
QODBC
```

depending on your Qt installation.

For plugin diagnostics:

### Linux / macOS

```bash
QT_DEBUG_PLUGINS=1 ./MyApplication
```

### Windows PowerShell

```powershell
$env:QT_DEBUG_PLUGINS = "1"
.\MyApplication.exe
```

Check:

* Qt version
* Compiler
* Architecture
* Debug/Release configuration
* SQL plugin location
* Native client libraries
* Runtime search paths

---

# ⚠️ Common Problems

| Problem                     | Check                                                      |
| --------------------------- | ---------------------------------------------------------- |
| `Q1ORM.h` not found         | Verify the `include/` directory                            |
| Q1ORM linker error          | Check architecture, compiler, Qt and library configuration |
| `Q1ORM.dll` not found       | Place the DLL beside the application executable            |
| Qt SQL driver not loaded    | Check `QSqlDatabase::drivers()`                            |
| SQLite cannot open database | Check path and directory permissions                       |
| PostgreSQL connection fails | Check host, port, credentials and `QPSQL`                  |
| MySQL driver not loaded     | Check `QMYSQL` and native MySQL client libraries           |
| SQL Server connection fails | Check `QODBC` and SQL Server ODBC driver                   |
| Schema initialization fails | Check `GetLastError()`                                     |
| SQLite database locked      | Check concurrent writers and long transactions             |
| Qt thread mismatch          | Create/use/destroy the connection in its owning thread     |

---

# 🚚 Deployment

A Q1ORM application needs more than the Q1ORM shared library.

For Windows, deploy:

```text
MyApplication.exe
Q1ORM.dll
Qt6Core.dll
Qt6Sql.dll
platforms/
sqldrivers/
```

and the native database client dependencies required by the selected backend.

For Linux and macOS, ensure the runtime loader can locate:

```text
libQ1ORM.so
```

or:

```text
libQ1ORM.dylib
```

along with the required Qt libraries and SQL plugins.

For SQLite, make sure the application has permission to create/open the database file.

Keep database credentials outside source control.

---

# 🔒 Security Guidelines

Q1ORM supports parameter binding for values.

Use:

```cpp
.Where(
    "name",
    Q1Operator::Equal,
    userProvidedName
)
```

instead of constructing SQL manually:

```cpp
// Do not do this.
QString sql =
    "SELECT * FROM users WHERE name = '" +
    userProvidedName +
    "'";
```

Raw SQL remains useful, but raw SQL syntax must be trusted.

Do not directly expose user-controlled values as:

* SQL identifiers
* Column names
* Table names
* SQL expressions
* `ORDER BY` expressions
* Join expressions

If an application allows users to select a column, translate the user selection through an explicit allowlist.

---

# 🏛️ Recommended Application Architecture

Q1ORM does not provide or require a dependency-injection container.

A simple application structure is:

```text
Application
│
├── Models/
│   ├── Country.h
│   └── City.h
│
├── Data/
│   └── ApplicationDbContext.h
│
├── Services/
│   ├── CountryService.h
│   └── CityService.h
│
└── main.cpp
```

Responsibilities:

```text
Model
  ↓
Mapping
  ↓
Q1Entity
  ↓
Q1Context
  ↓
Service
  ↓
Application
```

Keep business validation and application rules in services rather than putting unrelated business logic into the ORM mapping layer.

---

# 🔄 Application Startup Pattern

A typical application can follow:

```cpp
Q1Connection connection(...);

ApplicationDbContext context(connection);

if (!context.Initialize())
{
    qCritical() << context.GetLastError();
    return 1;
}

// Start application services
// Start workers
// Handle requests
```

Initialize the schema before launching worker threads or starting concurrent database activity.

---

# 📋 Complete CRUD Overview

| Operation          | Q1ORM API                         |
| ------------------ | --------------------------------- |
| Initialize schema  | `Initialize()`                    |
| Insert             | `Insert()`                        |
| Bulk insert        | `InsertRange()`                   |
| Select             | `Select()`                        |
| Filter             | `Where()`                         |
| AND                | `AndWhere()`                      |
| OR                 | `OrWhere()`                       |
| Sort               | `OrderBy()`                       |
| Pagination         | `Skip()` / `Take()`               |
| Count              | `Count()`                         |
| Update by ID       | `UpdateById()`                    |
| Bulk update        | `UpdateRange()`                   |
| Delete by ID       | `DeleteById()`                    |
| Bulk delete        | `DeleteRange()`                   |
| Join               | Query join API                    |
| Eager loading      | `Include()`                       |
| Group              | `GroupBy()`                       |
| Filter groups      | `Having()`                        |
| Aggregates         | Count / Min / Max / Sum / Average |
| JSON               | `ToJson()`                        |
| Transactions       | `Transaction()`                   |
| Commit             | `Commit()`                        |
| Transaction status | `InTransaction()`                 |
| Error              | `GetLastError()`                  |

---

# 🧭 Design Philosophy

Q1ORM follows a simple principle:

> **The C++ model describes what the application expects; the database catalog describes what currently exists.**

At startup, Q1ORM compares the two and applies supported schema changes.

This avoids the need for:

```text
Migration001
Migration002
Migration003
Migration004
...
```

for the supported automatic schema reconciliation workflow.

At the same time, Q1ORM deliberately does not guess destructive or ambiguous operations such as arbitrary renames and type conversions.

---

# 📚 Examples

The repository contains examples covering:

```text
Examples/
├── SoloExample/
├── DatabaseInstallExample/
├── UnitTestExample/
└── ...
```

The examples demonstrate database configuration, model mapping, CRUD, queries, schema initialization, relationships, testing and deployment patterns.

---

# 🤝 Contributing

Contributions are welcome.

Before submitting a pull request:

1. Build Q1ORM.
2. Build the examples.
3. Run the SQLite test suite.
4. Run relevant server-backend tests.
5. Add regression tests for bug fixes.
6. Update documentation for public API changes.
7. Review staged files for credentials and generated files.

For bug reports, include:

* Q1ORM version or commit
* Operating system
* Architecture
* Compiler
* Qt version
* Database backend
* Database server version
* Minimal reproducer
* Expected behavior
* Actual behavior
* Relevant sanitized logs

---

# 📄 License

Q1ORM is distributed under the **MIT License**.

See [`LICENSE`](LICENSE) for the complete license text.

Qt and database client libraries are separate dependencies and have their own licenses.

---

# ⭐ Support the Project

If Q1ORM is useful to you:

* ⭐ Star the repository
* 🐛 Report bugs
* 💡 Open feature requests
* 🔧 Submit pull requests
* 📖 Improve the documentation

---

<p align="center">
  <strong>Q1ORM — C++ database access without the boilerplate.</strong>
</p>
