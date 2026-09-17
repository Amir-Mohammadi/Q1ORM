# Q1ORM

<p align="center">
  <img src="Images/q1orm.png" alt="Q1ORM banner" width="900" />
</p>
<p align="center">
  <img src="https://img.shields.io/badge/C%2B%2B-17%2B-blue?style=for-the-badge" />
  <img src="https://img.shields.io/badge/Qt-Compatible-green?style=for-the-badge" />
  <img src="https://img.shields.io/badge/PostgreSQL-Supported-316192?style=for-the-badge" />
  <img src="https://img.shields.io/badge/SQL%20Server-Supported-CC2927?style=for-the-badge" />
  <img src="https://img.shields.io/badge/SQLite-Supported-003B57?style=for-the-badge" />
  <img src="https://img.shields.io/badge/MySQL-Supported-4479A1?style=for-the-badge" />
</p>

**Qt 6 ORM for C++20** with typed mappings, automatic schema initialization,
fluent queries, CRUD operations, relations, change tracking, and transactions.

Q1ORM separates plain C++ records, database mappings, and application services.
Register entities in a context, configure their mappings, initialize the
schema, and work with your database through a consistent API.


## Features

- **Typed mappings** — configure tables, keys, properties, and relationships.
- **Automatic schema initialization** — create missing schema objects and
  reconcile mappings.
- **Fluent queries** — filter, sort, paginate, join, group, and aggregate.
- **CRUD and bulk operations** — insert, update, and delete records or ranges.
- **Relations** — configure relationships and eagerly load related data.
- **Change tracking** — track snapshots, identify dirty fields, and update
  changed properties.
- **Transactions** — explicit commits and scope-based rollback.
- **Multiple backends** — SQLite, PostgreSQL, MySQL, and SQL Server.
- **Cross-platform usage** — Windows, Linux, and macOS with compatible Qt kits.
- **Automated tests** — functional, service, concurrency, and schema-scale checks.

Backend capabilities and SQL behavior differ. A supported driver does not mean
every query or schema operation behaves identically across all databases.

**One example/test project: `Examples/TestExample`.** It demonstrates ORM and
service usage with automated assertions and clear PASS/FAIL results.

## Contents

- [Requirements](#requirements)
- [Create the release package](#create-the-release-package)
- [Test results and coverage](#test-results-and-coverage)
- [Database connections](#database-connections)
- [Use Q1ORM in your application](#use-q1orm-in-your-application)
- [Complete quick start](#complete-quick-start)
- [Application services](#application-services)
- [Transactions and threading](#transactions-and-threading)
- [Schema changes and data safety](#schema-changes-and-data-safety)
- [Error handling and diagnostics](#error-handling-and-diagnostics)
- [Deployment checklist](#deployment-checklist)
- [Troubleshooting](#troubleshooting)
- [Contributing](#contributing)
- [GitHub and continuous integration](#github-and-continuous-integration)
- [License](#license)
](#github-and-continuous-integration)
- [License](#license)
 or newer, subject to the requirements of your selected Qt version.
- Qt 6:
  - **Core** and **Sql** for applications.
  - **Test** for the example/test project.
- A C++20 compiler and standard library with `std::barrier` support.
  Suitable toolchains include GCC 11+, Visual Studio 2022, or a recent
  Apple Clang/libc++ combination with the required support.
- A Qt SQL plugin for the selected database backend.
- For server databases, the corresponding client libraries and a reachable
  database server.

Use matching operating systems, architectures, compiler toolchains, build
configurations, and Qt kits when building the library and consuming applications.

## Create the release package

Download or clone the repository, then run the release script for your system.

The scripts configure, build, and install Q1ORM. No separate manual CMake build
commands are needed for the scripted release process.

### Linux

On Ubuntu, install the prerequisites:

```bash
sudo apt update
sudo apt install build-essential cmake qt6-base-dev libqt6sql6-sqlite
```
From the repository directory:

```bash
chmod +x release.sh
./release.sh
```
The script builds in `build-ubuntu/`.

If Qt is not detected automatically, set `QTDIR` to your Qt installation before
running the script:

```bash
export QTDIR="/path/to/Qt/gcc_64"
./release.sh
```

### Windows

Install:

- Visual Studio 2022 with **Desktop development with C++**.
- CMake.
- Qt 6 for **MSVC 2022 64-bit**.

Match the compiler and Qt architecture.

Run the release script from the repository directory:

```bat
release.bat
```
You can also double-click `release.bat`.

The script detects Qt and MSVC and builds in `build/`. If Qt is not detected
automatically, set `QTDIR` first:

```bat
set "QTDIR=C:\Qt\<version>\msvc2022_64"
release.bat
```

### Generated release folder

Both scripts create or update:

```text
Releases/
└── Release-0.1/
├── include/     Q1ORM headers
├── lib/         Linux shared library or Windows import library
├── bin/         Windows Q1ORM.dll and release tools
└── scripts/     Database installation helper
```
Use this folder in your own executable project. Applications consuming the
release package do not need to compile Q1ORM sources again.

Re-run the release script after changing the library so the packaged headers
and binaries stay synchronized.

> **A successful release build is not an all-tests-passed result.**
> The scripts build the tests but do not run the test suite.

### macOS

Install Xcode Command Line Tools, CMake, and a matching Qt 6 kit.

The current `release.sh` targets Linux and uses `nproc`; it is not a macOS
release script.

On macOS:

1. Open the root `CMakeLists.txt` in Qt Creator.
2. Select your macOS Qt kit.
3. Build the Release configuration.
4. Build the CMake `install` target.

The installation populates `Releases/Release-0.1/` with headers and
`lib/libQ1ORM.dylib`.

For the test commands below, replace `build-ubuntu` with your actual macOS
CMake build directory.

## Test results and coverage

After running the Linux release script:

```bash
./build-ubuntu/bin/TestExample
```

On Windows:

```powershell
.\build\bin\Release\TestExample.exe
```

Qt Test prints PASS/FAIL results and totals.

### Run with CTest

Run all registered tests:

```bash
ctest --test-dir build-ubuntu --output-on-failure
```
Run the threaded CRUD test with verbose output:

```bash
ctest --test-dir build-ubuntu -R Q1ORM.threadedCrud -V
```
Run tests labeled `scale`:

```bash
ctest --test-dir build-ubuntu -L scale --output-on-failure
```
For Windows, replace `build-ubuntu` with `build` and add `-C Release`:

```powershell
ctest --test-dir build -C Release --output-on-failure
```

### Select tests and generate reports

List available test functions:

```bash
./build-ubuntu/bin/TestExample -functions
```
Run selected functions:

```bash
./build-ubuntu/bin/TestExample crud aggregates
```
Generate a JUnit XML report:

```bash
./build-ubuntu/bin/TestExample -o results.xml,junitxml
```
CTest returns zero only when every selected test passes. Failed assertions,
missing plugins, initialization errors, and connection failures produce a
failing result.

Filtered runs prove only their selected coverage. Set `Q1ORM_VERBOSE=1` for
SQL logs.

### Coverage

| Test | Checks |
| --- | --- |
| `crud` | Generated IDs, persisted inserts/updates, deletion |
| `bulkCrud` | `InsertRange`, distinct IDs, `UpdateRange`, `DeleteRange` |
| `selects` | Filters, OR, ordered values, limit, skip/take, empty results |
| `boundValues` | Quotes, Unicode, SQL-looking text treated as values |
| `aggregates` | Exact count/min/max/sum/average, distinct and filtered aggregates |
| `grouping` | `GroupBy`/`Having` and actual group counts |
| `joinsAndIncludes` | Inner/left joins, unmatched parent, eager-loading values; right/full joins on applicable server backends |
| `jsonAndRawSql` | JSON values, raw scalar SQL and query rows |
| `changeTracking` | Snapshots, dirty fields, changed-field update, clear tracking |
| `transactions` | Commit persistence, automatic/explicit/nested rollback |
 orphan `constraints` | Reject orphan insertion and verify default cascade deletion |
| `errorHandling` | Reject unfiltered writes, report bad SQL, roll back failed bulk insert |
| `sqlGeneration` | Identity SQL for all four dialects without servers |
| ` dialects without servers |
| `connectionLifecycle` | Reference-counted openthreadedCrud` | Four simultaneous workers, ten ORM CRUD cycles each, final count |
| `services` | CityService validation, persistence, sorted lookup |
| `modelBuilder` | Typed/legacy maps, invalid mappings, relations, schema reconciliation |
| `schemaScale` | 300 tables, 30,000 rows, 599 indexes, 299 foreign keys, schema evolution |

SQLite is the default backend and uses automatically removed temporary files.
Ordinary tests reseed known data independently.

The generated 300-model fixture remains in `SchemaScale/` and is built into the
same executable.

These tests cover the listed behaviors, not every possible input or backend.
The coverage table is not a claim that tests have passed for a particular
checkout; run the suite or inspect CI results for that commit.

## Database connections

| Database | Q1Driver | Qt plugin | Default port |
| --- | --- | --- | --- |
| SQLite | `Q1Driver::SQLITE` | `QSQLITE` | — |
| PostgreSQL | `Q1Driver::POSTGRE_SQL` | `QPSQL` | 5432 |
| MySQL | `Q1Driver::MYSQL` | `QMYSQL` | 3306 |
| SQL Server | `Q1Driver::SQLSERVER` | `QODBC` | 1433 |

SQLite needs no database server.

Other backends require:

- A reachable server.
- An existing database and a database account.
- Appropriate schema and data permissions.
- The matching Qt SQL plugin.
- Required native client libraries.

Ubuntu plugin packages include:

```bash
sudo apt install libqt6sql6-psql
sudo apt install libqt6sql6-mysql
sudo apt install libqt6sql6-odbc
```
Install only the packages required for your selected backends.

On Windows and macOS, install or build plugins for the exact Qt/compiler kit.
SQL Server additionally requires its ODBC driver.

Inspect `QSqlDatabase::drivers()` and use `QT_DEBUG_PLUGINS=1` to diagnose
plugin-loading problems.

### Connection examples

The application API is identical on Windows, Linux, and macOS:

```cpp
Q1Connection sqlite(
Q1Driver::SQLITE,
"",
"application.sqlite",
"",
"",
0
);

Q1Connection postgres(
Q1Driver::POSTGRE_SQL,
"localhost",
"application",
qEnvironmentVariable("DB_USER"),
qEnvironmentVariable("DB_PASSWORD"),
5432
);

Q1Connection mysql(
Q1Driver::MYSQL,
"localhost",
"application",
qEnvironmentVariable("DB_USER"),
qEnvironmentVariable("DB_PASSWORD"),
3306
);

Q1Connection sqlServer(
Q1Driver::SQLSERVER,
"localhost",
"application",
qEnvironmentVariable("DB_USER"),
qEnvironmentVariable("DB_PASSWORD"),
1433
);
```

`Q1ORM_SQLSERVER_ODBC_DRIVER` selects an installed SQL Server ODBC driver.
The SQL Server host argument also accepts a DSN or ODBC connection string.

Keep credentials outside source control.

### Run tests on a server

> **Use a dedicated disposable database.**
> Tests initialize schema and delete/reseed all rows in `test_cities` and
> `test_countries`. They leave test data in server databases.
>
> Do not use an application or production database, and do not run multiple
> test processes against the same server test database.

Linux:

```bash
export Q1ORM_TEST_DRIVER=POSTGRES
export Q1ORM_DB_HOST=localhost
export Q1ORM_DB_NAME=q1orm_test
export Q1ORM_DB_USER=q1orm_test
export Q1ORM_DB_PASSWORD='<your-test-database-password>'
export Q1ORM_DB_PORT=5432

ctest --test-dir build-ubuntu --output-on-failure
```
On macOS, use the same environment variables and substitute your build directory.

Windows PowerShell:

```powershell
$env:Q1ORM_TEST_DRIVER = "POSTGRES"
$env:Q1ORM_DB_HOST = "localhost"
$env:Q1ORM_DB_NAME = "q1orm_test"
$env:Q1ORM_DB_USER = "q1orm_test"
$env:Q1ORM_DB_PASSWORD = "<your-test-database-password>"
$env:Q1ORM_DB_PORT = "5432"

ctest --test-dir build -C Release --output-on-failure
```
Supported selections:

- `SQLITE` — default.
- `POSTGRES`
- `MYSQL`
- `SQLSERVER`

Omit the port to use its default. Server database names are required; usernames
and passwords have no defaults.

A selected unavailable backend fails instead of skipping or falling back.

`modelBuilder` and `schemaScale` always use SQLite.

To return to the default, unset `Q1ORM_TEST_DRIVER` or set it to `SQLITE`.

Linux/macOS:

```bash
unset Q1ORM_TEST_DRIVER
```
Windows PowerShell:

```powershell
Remove-Item Env:Q1ORM_TEST_DRIVER -ErrorAction SilentlyContinue
```
## Use Q1ORM in your application

Run the release script first, then copy:

```text
Releases/Release-0.1/
```
into your application as:

```text
external/Q1ORM/Release-0.1/
```
A minimal application layout is:

```text
MyApplication/
├── CMakeLists.txt
├── main.cpp
└── external/
└── Q1ORM/
└── Release-0.1/
├── include/
├── lib/
├── bin/
└── scripts/
```
### CMake integration

Import the compiled library in your application's `CMakeLists.txt`:

```cmake
cmake_minimum_required(VERSION 3.14)

project(MyApplication LANGUAGES CXX)

find_package(Qt6 REQUIRED COMPONENTS Core Sql)

set(
Q1ORM_RELEASE
"${CMAKE_CURRENT_SOURCE_DIR}/external/Q1ORM/Release-0.1"
)

add_library(Q1ORM::Q1ORM SHARED IMPORTED)

set_target_properties(Q1ORM::Q1ORM PROPERTIES
INTERFACE_INCLUDE_DIRECTORIES "${Q1ORM_RELEASEQ1ORM PROPERTIES
INTERFACE_INCLUDE_DIRECTORIES "${Q1ORM_RELEASE    INTERFACE_COMPILE_FEATURES cxx_std_20    INTERFACE
Qt6::Core
Qt6::Sql
)

if(WIN32)
set_target_properties(Q1ORM::Q1ORM PROPERTIES
IMPORTED_LOCATION "${Q1ORM_RELEASE}/bin/Q1ORM.dll"
IMPORTED_IMPLIB "${Q1ORM_RELEASE}/lib/Q1ORM.lib"
)
elseif(APPLE)
set_target_properties(Q1ORM::Q1ORM PROPERTIES
IMPORTED_LOCATION "${Q1ORM_RELEASE}/lib/libQ1ORM.dylib"
)
else()
set_target_properties(Q1ORM::Q1ORM PROPERTIES
_RELEASEED_LOCATION "${Q1ORM_RELEASE}/lib/libQ1ORM.so"
)
endif()

add_executable(MyApplication main.cpp)

target_link_libraries(MyApplication
PRIVATE
Q1ORM::Q1ORM
)

if(WIN32)
add_custom_command(TARGET MyApplication POST_BUILD
COMMAND ${CMAKE_COMMAND} -E copy_if_different
"$<TARGET_FILE:Q1ORM::Q1ORM>"
"$<TARGET_FILE_DIR:MyApplication>"
VERBATIM
)
endif()
```
The imported target propagates:

- Q1ORM include paths.
- The C++20 requirement.
- Qt Core and Sql dependencies.

This configuration imports one packaged library configuration. Build the
application with a compatible configuration; use. Build the
application with a compatible configuration; use separate imported locations
if you maintain

For another build system:

1. Add the release `include/` directory to the header search paths.
2. Link the Q1ORM library from `lib/`.
3. Link Qt Core and Sql.
4. Enable C++20.
5. Deploy the required runtime libraries and plugins.

Windows applications need `Q1ORM.dll` beside the executable, along with the
required Qt runtime DLLs and plugins.

On Linux and macOS, the runtime loader must be able to locate Q1ORM and Qt.
Server SQL plugins also need their native client dependencies.

The repository's `TestExample` links the source library to test current changes.
The imported target above is for applications consuming the release package.

## Complete quick start

Save the following as `main.cpp` and use the CMake configuration above.

The example:

1. Creates a Qt application object.
2. Opens a SQLite-backed context.
3. Initializes the mapped schema.
4. Inserts and updates a country.
5. Filters, sorts, and paginates query results.
6. Counts records.
7. Deletes the inserted record.
8. Commits the transaction.

```cpp
#include <QCoreApplication>
#include <QDebug>
#include <QString>

#include <Q1ORM.h>


struct Country
{
int id = 0;
QString name;

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

class AppContext : public Q1Context
{
public:
explicit AppContext(Q1Connection& connection)
{
// The caller retains ownership of the connection.
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

// A relative SQLite path is resolved from the working directory.
// The connection must outlive the context.
Q1Connection connection(
Q1Driver::SQLITE,
"",
"application.sqlite",
"",
"",
0
);

AppContext context(connection);

if (!context.Initialize())
{
qCritical() << "Schema initialization failed:"
<< context.GetLastError();
return 1;
}

auto transaction = connection.Transaction();

if (!connection.InTransaction())
{
qCritical() << "Could not start the transaction.";
return 1;
}

Country country{0, QStringLiteral("Canada")};

if (!context.countries.Insert(country))
{
qCritical(country))
{
qCritical() << "Insert failed.";
returnInfo() << "Generated ID:" << country.id;

country.name = QStringLiteral("Canada - updated");

if (!context.countries.UpdateById(country, country.id))
{
qCritical() << "Update failed.";
return 1;
qCritical() << "Update failed.";
return 1;
Where("name", Q1Operator::Equal, country.name)
.OrderBy("id", Q1Sort::Ascending)
.Skip(0)
.Take(10)
.ToList();

for (const auto& row : rows)
qInfo() << row.id << row.name;

const auto count = context.countries.Select().Count();
qInfo() << "Country count before deletion:" << count;

if (!context.countries.DeleteById(country.id))
{
qCritical() << "Delete failed.";
return 1;
}

if (!transaction.Commit())
{
qCritical() << "Commit failed.";
return  << "Commit failed.";
return 1;
}

qInfo completed.";
return 0;
}
```
The inserted record is deleted before commit. Existing rows are not cleared.

Uncommitted writes are rolled back when the transaction guard leaves scope.
Schema initialization occurs before the transaction, so rolling back the CRUD
operations does not undo that earlier initialization.

For deployed applications, use a writable application-data directory rather
than relying on the working directory.

### Mapping structure

The example separates responsibilities:

| Component | Responsibility |
| --- | --- |
| `Country` | Plain C++ record |
| `CountryMap` | Table and property mapping |
| `AppContext` | Connection association, entity registration, model configuration |
| `Q1Entity<Country>` | Entity operations and query entry point |

Include `<Q1ORM.h>` and create a `QCoreApplication` or an appropriate Qt GUI
application object before using Qt SQL.

### Query safety

Use bound values for user-provided data:

```cpp
const QString requestedName = QStringLiteral("Canada");

const auto rows = context.countries.Select()
.Where("name", Q1Operator::Equal, requestedName)
.OrderBy("id", Q1Sort::Ascending)
.ToList();
```
Do not concatenate user input into raw SQL.

Raw SQL clauses, identifiers, join expressions, and aggregate expressions must
be trusted SQL. Binding protects values, not SQL syntax or identifiers.

If users can select a sort column or another identifier, map their selection
to an explicit allowlist.

## Application services

Q1ORM has no dependency-injection container. Create small services that receive
a context and encapsulate application rules.

The example project separates:

| File | Purpose |
| --- | --- |
| `Models.h` | Plain records |
| `ApplicationContext.h` | Typed maps and entity sets |
| `CityService.h` | Business logic and validation |
| `OrmTests.cpp` | Automated assertions |
| `main.cpp` | Test runner |

With the example headers and an existing connection:

```cpp
ApplicationContext context(connection);

if (!context.Initialize())
{
qCritical() << context.GetLastError();
return 1;
}

CityService service(context);

Country country{0, "Canada"};

if (!context.countries.Insert(country))
return 1;

City city{0, "Montreal", country.id, 100};

if (!service.Create(city))
{
qCritical() << service.LastError();
return 1;
}

const auto cities = service.InCountry(country.id);
```
The service trims names, validates population and country, and persists through
the ORM. Tests cover successful writes and rejected requests.

Keep the context alive while a service uses it, and keep the underlying
connection alive while the context uses it.

## Transactions and threading

### Transactions

Create a transaction guard, check that the transaction started, perform the
operations, and commit explicitly:

```cpp
auto transaction = connection.Transaction();

if (!connection.InTransaction())
return 1;

Country country{0, "Transactional"};

if (!context.countries.Insert(country))
return 1;

if (!transaction.Commit())
return 1;
```
Uncommitted guards roll back at scope exit, including early returns.

Nested transactions join the outer transaction.

Nested transactions join the outer transaction. An inner rollback prevents
the guards as independent transactions.

Keep transactions short and check commit results.

### Threading

Create and destroy a separate connection, context, and service inside each
worker thread.

- Initialize schema before launching workers.
- Do not share Qt SQL handles or entity sets across threads.
- Perform database operations in the thread that owns the connection.
- Finish active operations before destroying the context and connection.
- Expect backend-specific locking and concurrent-write behavior.

`threadedCrud` demonstrates simultaneous ORM reads and writes against the same
database using independent connections.

Thread-local connections do not remove database-level contention. In
particular, concurrent SQLite writers may encounter locking.

## Schema changes and data safety

`Initialize()` creates missing schema objects and reconciles mappings.

Always check its result:

```cpp
if (!context.Initialize())
{
qCritical() << context.GetLastError();
return 1;
}
```
Automatic reconciliation should not be assumed to provide a versioned migration
history or to infer every intended schema transformation.

Before changing mappings for an existing database:

1. Back up the database and verify that it can be restored.
2. Test the changes against a copy of representative data.
3. Review required columns, defaults, keys, indexes, and relationships.
4. Check initialization errors and stop startup on failure.
5. Validate important queries and existing records after the change.

A required new column without a default can fail on populated tables.
Some changes may require an explicit data backfill.

Handle renames, type conversions, and destructive changes deliberately rather
than assuming they are inferred automatically.

Schema-alteration capabilities and transactional-DDL behavior vary by backend.
Test changes using the same database engine used in deployment.

Coordinate schema initialization before worker startup or concurrent
application instances attempt schema changes.

## Error handling and diagnostics

Check return values before continuing with dependent work, especially for:

- Context initialization.
- Inserts, updates, and deletes.
- Bulk operations.
- Transaction startup and commit.

### SQL logging

Enable verbose SQL logging before starting the application.

Linux/macOS:

```bash
Q1ORM_VERBOSE=1 ./MyApplication
```
Windows PowerShell:

```powershell
$env:Q1ORM_VERBOSE = "1"
.\MyApplication.exe
```
Treat SQL logs as potentially sensitive. Remove credentials, connection strings,
personal data, and confidential values before sharing them.

### SQL plugin diagnostics

Inspect available drivers:

```cpp
#include <QSqlDatabase>

qInfo() << "Available SQL drivers:" << QSqlDatabase::drivers();
```
Enable plugin diagnostics when a driver cannot be loaded.

Linux/macOS:

```bash
QT_DEBUG_PLUGINS=1 ./MyApplication
```
Windows PowerShell:

```powershell
$env:QT_DEBUG_PLUGINS = "1"
.\MyApplication.exe
```
Check the plugin's architecture, Qt compatibility, runtime search paths, and
native client dependencies.

## Deployment checklist

Before distributing an application:

- [ ] Build Q1ORM and the application with compatible toolchains and Qt kits.
- [ ] Match operating system, architecture, and build configuration.
- [ ] Rebuild the release package after library changes.
- [ ] Deploy the Q1ORM shared library.
- [ ] Deploy required Qt runtime libraries.
- [ ] Deploy the selected Qt SQL plugin.
- [ ] Install or deploy native database client dependencies where required.
- [ ] Verify runtime library search paths.
- [ ] Use a writable database location for SQLite.
- [ ] Keep credentials outside source control.
- [ ] Assign appropriate database permissions.
- [ ] Test schema changes against representative data.
- [ ] Run tests for the backend used in deployment.
- [ ] Test on a machine without the development environment.

Copying `Q1ORM.dll` alone is not a complete Windows deployment.
Similarly, shipping `libQ1ORM.so` or `libQ1ORM.dylib` does not automatically
deploy Qt, SQL plugins, or database client libraries.

## Troubleshooting

| Problem | What to check |
| --- | --- |
| CMake cannot find Qt | Confirm the Qt kit. Set `QTDIR` for the release scripts; for direct CMake configuration, set `CMAKE_PREFIX_PATH` or `Qt6_DIR` as appropriate. |
| `std::barrier` is unavailable | Confirm C++20 is enabled and both the compiler and standard library support it. |
| `Q1ORM.h` cannot be found | Verify the packaged `include/` directory and the imported target's include path. |
| Linker errors or incompatible library format | Check architecture, compiler ABI, Qt kit, build configuration, and library paths. |
| Q1ORM is not found at startup | Check DLL placement on Windows or runtime library search paths on Linux/macOS. |
| Qt SQL driver is not loaded | Inspect `QSqlDatabase::drivers()`, enable `QT_DEBUG_PLUGINS=1`, and check plugin dependencies. |
| Server connection fails | Check host, port, database name, authentication, network access, and client/ODBC configuration. |
| SQLite cannot open the database | Check the resolved path, parent-directory existence, and filesystem permissions. |
| Schema initialization fails | Inspect `GetLastError()`, permissions, existing records, and incompatible mappings. |
| SQLite reports a locked database | Check concurrent writers and long-running transactions. Avoid overlapping test processes. |
| Qt reports a connection/thread mismatch | Create, use, and destroy the connection and context in the same worker thread. |
| SQLite tests pass but server tests fail | Check backend capabilities, plugin availability, permissions, and backend-specific SQL behavior. |
| Release build succeeds but tests are unverified | Run `TestExample` or CTest explicitly. Release scripts do not run the suite. |

## Contributing

Keep changes focused and include tests for new behavior and bug fixes.

Before submitting a change:

1. Build Q1ORM and `Examples/TestExample`.
2. Run the full SQLite test suite.
3. Run relevant server-backend tests for changes affecting connections,
   dialects, schema generation, or backend-specific behavior.
4. Add regression assertions for bug fixes.
5. Update documentation for public API or configuration changes.
6. Review staged files for credentials, local databases, logs, and unintended
   generated artifacts.

When reporting a problem, include:

- Q1ORM commit or release.
- Operating system and architecture.
- Compiler and Qt versions.
- Database backend and server version, if applicable.
- A minimal reproducer.
- Expected and actual behavior.
- Relevant sanitized diagnostics.

State which backends and test selections you ran. A passing filtered test run
verifies only that selection.

## GitHub and continuous integration

`.github/workflows/ci.yml` builds and tests SQLite on Linux, Windows, and macOS
for pushes and pull requests, uploading test logs even on failure.

Check GitHub Actions for the exact commit being evaluated.

A green SQLite run does not certify server backends. Run PostgreSQL, MySQL,
and SQL Server tests explicitly using the environment settings above.

### Publish changes

Review changes before environment settings above.

### Publish changes

Review changes before diff

```bash
git add -A
git diff --cached

git commit -m "Consolidate ORM examples and automated tests"
git push origin HEAD
```

These commands publish source to the existing remote. They do not deploy a
hosted application or database.

## License

Q1ORM is distributed under the MIT License. See `LICENSE` for the full terms.

Qt and database client libraries are separate dependencies with their own
licenses. Review the applicable terms when distributing your application.

## References

- [Qt CMake setup](https://doc.qt.io/qt-6/cmake-get-started.html)
- [Qt SQL plugins](https://doc.qt.io/qt-6/sql-driver.html)
- [Qt Windows deployment](https://doc.qt.io/qt-6/windows-deployment.html)
- [MIT license](LICENSE)
`

