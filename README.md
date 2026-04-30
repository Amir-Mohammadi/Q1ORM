# Q1ORM

Q1ORM is a Qt-based ORM for C++ projects that work with PostgreSQL or SQL Server. It helps you define entities, map them to tables, initialize the database schema, and run fluent queries such as `Select`, `Where`, `Join`, `GroupBy`, `Having`, `OrderBy`, and aggregate functions.

This README is written to be GitHub-friendly, so you can push the project and have a clear landing page for build steps and usage.

## Features

- Qt-friendly API built around `Q1Connection`, `Q1Context`, `Q1Entity<T>`, and `Q1Query<T>`
- Supports PostgreSQL and SQL Server
- Creates databases, tables, columns, and relations during `Initialize()`
- Fluent query builder for filtering, sorting, joins, grouping, eager loading, and aggregates
- JSON and table-style output for debugging
- Example applications and unit tests included

## Project structure

- `src/` - the main Q1ORM library
- `Examples/SoloExample/` - a simple end-to-end usage example
- `Examples/DatabaseInstallExample/` - database install helper example
- `Examples/UnitTestExample/` - integration and SQL-generation tests
- `Docs/` - extra documentation
- `Releases/Release-0.1/` - installed library layout used by the examples
- `Tools/` - helper tools for release packaging and example setup

## Requirements

Before building, make sure you have:

- CMake 3.14 or newer
- Qt 5 or Qt 6 with `Core` and `Sql`
- A C++20 compatible compiler
- A PostgreSQL server or SQL Server instance if you want to run the database examples/tests
- For SQL Server, a working ODBC driver such as `ODBC Driver 17 for SQL Server`

## Build

Build the whole project:

```bash
cmake -S . -B build
cmake --build build
```

If Qt is not auto-detected on your machine, pass your Qt path with `CMAKE_PREFIX_PATH`:

```bash
cmake -S . -B build -DCMAKE_PREFIX_PATH="C:/Qt/6.5.3/msvc2019_64"
cmake --build build
```

Install the built library into the release layout used by the examples:

```bash
cmake --install build
```

That produces a structure like:

```text
Releases/Release-0.1/
  bin/
  lib/
  include/
  scripts/
```

## Run examples and tests

After building:

- `SoloExample` shows normal ORM usage
- `DatabaseInstallExample` shows the installer helper
- `UnitTestExample` contains automated tests

If your generator supports CTest, you can run:

```bash
ctest --test-dir build --output-on-failure
```

## Database configuration

### Generic environment variables

`Examples/SoloExample/` reads these variables:

- `Q1ORM_DB_DRIVER=postgres` or `Q1ORM_DB_DRIVER=sqlserver`
- `Q1ORM_DB_HOST`
- `Q1ORM_DB_NAME`
- `Q1ORM_DB_USER`
- `Q1ORM_DB_PASSWORD`
- `Q1ORM_DB_PORT`

Example for PostgreSQL:

```bash
export Q1ORM_DB_DRIVER=postgres
export Q1ORM_DB_HOST=localhost
export Q1ORM_DB_NAME=q1orm_test
export Q1ORM_DB_USER=postgres
export Q1ORM_DB_PASSWORD=123
export Q1ORM_DB_PORT=5432
```

Example for SQL Server:

```bash
export Q1ORM_DB_DRIVER=sqlserver
export Q1ORM_DB_HOST=localhost
export Q1ORM_DB_NAME=q1orm_test
export Q1ORM_DB_USER=sa
export Q1ORM_DB_PASSWORD=123
export Q1ORM_DB_PORT=1433
export Q1ORM_SQLSERVER_ODBC_DRIVER="ODBC Driver 17 for SQL Server"
```

### Test-specific environment variables

`Examples/UnitTestExample/` also supports backend-specific settings:

- PostgreSQL: `Q1ORM_PG_HOST`, `Q1ORM_PG_DB_NAME`, `Q1ORM_PG_USER`, `Q1ORM_PG_PASSWORD`, `Q1ORM_PG_PORT`
- SQL Server: `Q1ORM_SQLSERVER_HOST`, `Q1ORM_SQLSERVER_DB_NAME`, `Q1ORM_SQLSERVER_USER`, `Q1ORM_SQLSERVER_PASSWORD`, `Q1ORM_SQLSERVER_PORT`
- Shared fallback: `Q1ORM_DB_HOST`, `Q1ORM_DB_USER`, `Q1ORM_DB_PASSWORD`, `Q1ORM_DB_PORT`, `Q1ORM_TEST_DB_NAME`

### SQL Server connection string support

For SQL Server, `Q1Connection` can also use a DSN or a full ODBC-style server string through `Q1ORM_DB_HOST` or `Q1ORM_SQLSERVER_HOST`. If the value already contains `Driver=` or `DSN=`, Q1ORM uses it as the base connection string.

## Quick start

The normal flow is:

1. Create model classes
2. Map them to tables with `Q1Entity<T>`
3. Create an application context from `Q1Context`
4. Configure a `Q1Connection`
5. Call `Initialize()`
6. Use CRUD and query methods

### 1. Define your models

```cpp
class Country
{
public:
    int id;
    QString name;
};

class City
{
public:
    int id;
    QString name;
    int country_id;
};
```

### 2. Map models to tables

```cpp
class CountryMap : public Q1Entity<Country>
{
public:
    static void ConfigureEntity(Q1Entity<Country>& entity)
    {
        entity.ToTableName("countries");
        entity.Property(entity.id, "id", false, true, "GENERATED ALWAYS AS IDENTITY");
        entity.Property(entity.name, "name", false, false);
    }

    static QList<Q1Relation> CreateRelations(Q1Entity<Country>& entity)
    {
        QList<Q1Relation> relations;
        relations.append(entity.Relations("countries", "cities", ONE_TO_MANY, "country_id", "id"));
        return relations;
    }
};
```

```cpp
class CityMap : public Q1Entity<City>
{
public:
    static void ConfigureEntity(Q1Entity<City>& entity)
    {
        entity.ToTableName("cities");
        entity.Property(entity.id, "id", false, true, "GENERATED ALWAYS AS IDENTITY");
        entity.Property(entity.name, "name", false, false);
        entity.Property(entity.country_id, "country_id", false, false);
    }

    static QList<Q1Relation> CreateRelations(Q1Entity<City>& entity)
    {
        QList<Q1Relation> relations;
        relations.append(entity.Relations("cities", "countries", MANY_TO_ONE, "country_id", "id"));
        return relations;
    }
};
```

### 3. Create your `DbContext`

```cpp
class ApplicationDbContext : public Q1Context
{
public:
    explicit ApplicationDbContext(Q1Connection* conn)
        : cities(conn),
          countries(conn)
    {
        SetConnection(conn, false);
    }

    void OnConfiguration() override;
    QList<Q1Table*> OnTablesCreating() override;
    QList<Q1Relation> OnTableRelationCreating() override;

    Q1Entity<City> cities;
    Q1Entity<Country> countries;
};
```

```cpp
void ApplicationDbContext::OnConfiguration()
{
    if (!connection)
    {
        SetConnection(new Q1Connection(
            Q1Driver::POSTGRE_SQL,
            "localhost",
            "q1orm_test",
            "postgres",
            "123",
            5432
        ), true);
    }
}

QList<Q1Table*> ApplicationDbContext::OnTablesCreating()
{
    CityMap::ConfigureEntity(cities);
    CountryMap::ConfigureEntity(countries);
    CityMap::CreateRelations(cities);
    CountryMap::CreateRelations(countries);

    QList<Q1Table*> tables;
    tables.append(cities.GetTablePtr());
    tables.append(countries.GetTablePtr());
    return tables;
}

QList<Q1Relation> ApplicationDbContext::OnTableRelationCreating()
{
    QList<Q1Relation> relations;
    relations += CityMap::CreateRelations(cities);
    return relations;
}
```

### 4. Connect and initialize

```cpp
Q1Connection* conn = new Q1Connection(
    Q1Driver::POSTGRE_SQL,
    "localhost",
    "q1orm_test",
    "postgres",
    "123",
    5432
);

ApplicationDbContext ctx(conn);

if (!ctx.Initialize())
{
    qWarning() << "Initialization failed:" << ctx.GetLastError();
    return;
}
```

`Initialize()` is responsible for:

- configuring the connection
- creating the database if needed
- creating missing tables
- adding missing columns
- creating relations

## CRUD usage

### Insert

```cpp
Country usa;
usa.name = "USA";

if (!ctx.countries.Insert(usa))
{
    qWarning() << ctx.countries.GetLastError();
}

City newYork;
newYork.name = "New York";
newYork.country_id = usa.id;
ctx.cities.Insert(newYork);
```

### Read

```cpp
QList<City> cities = ctx.cities.Select().ToList();
```

### Update

```cpp
Country country = ctx.countries.Select()
    .Where("id = 1")
    .ToList()
    .first();

country.name = "United States";
ctx.countries.UpdateById(country, country.id);
```

### Delete

```cpp
ctx.cities.DeleteById(1);
ctx.countries.Delete("id = 2");
```

## Query guide

Q1ORM query operations are available from `Q1Entity<T>::Select()`. You can chain methods fluently.

### Basic select

Select all columns:

```cpp
QList<City> cities = ctx.cities.Select().ToList();
```

Select specific columns:

```cpp
QList<City> cities = ctx.cities.Select({"id", "name"}).ToList();
```

### Where

```cpp
QList<City> usaCities = ctx.cities.Select()
    .Where("country_id = 1")
    .ToList();
```

### Order by

```cpp
ctx.cities.Select().OrderByAsc("name").ToList();
ctx.cities.Select().OrderByDesc("name").ToList();
ctx.cities.Select().OrderBy("id DESC").ToList();
```

### Limit

```cpp
QList<City> firstTwo = ctx.cities.Select()
    .OrderByAsc("name")
    .Limit(2)
    .ToList();
```

### Distinct

```cpp
int distinctCountries = ctx.cities.Select()
    .Distinct()
    .Count("country_id");
```

### Aggregate functions

```cpp
int totalCities = ctx.cities.Select().Count();
int maxId = ctx.cities.Select().Max<int>("id");
int minId = ctx.cities.Select().Min<int>("id");
int sumIds = ctx.cities.Select().Sum<int>("id");
double avgId = ctx.cities.Select().Avg<double>("id");
```

### Inner join

```cpp
ctx.cities.Select({"cities.name AS city", "countries.name AS country"})
    .InnerJoin("countries", "cities.country_id = countries.id")
    .OrderByAsc("cities.name")
    .ToList();
```

### Left join

```cpp
ctx.cities.Select({"cities.name AS city", "countries.name AS country"})
    .LeftJoin("countries", "cities.country_id = countries.id")
    .ToList();
```

### Right join

```cpp
ctx.countries.Select({"countries.name AS country", "cities.name AS city"})
    .RightJoin("cities", "countries.id = cities.country_id")
    .ToList();
```

### Full outer join

```cpp
ctx.cities.Select({"cities.name AS city", "countries.name AS country"})
    .FullJoin("countries", "cities.country_id = countries.id")
    .ToList();
```

### Group by

```cpp
ctx.cities.Select({"countries.name AS country", "COUNT(*) AS city_count"})
    .InnerJoin("countries", "cities.country_id = countries.id")
    .GroupBy("countries.name")
    .ToList();
```

### Having

```cpp
ctx.cities.Select({"countries.name AS country", "COUNT(*) AS city_count"})
    .InnerJoin("countries", "cities.country_id = countries.id")
    .GroupBy("countries.name")
    .Having("COUNT(*) > 1")
    .ToList();
```

### Include

`Include()` eager-loads related data. It keeps the typed entity list and also stores the related data in the last JSON result.

```cpp
QList<Country> countries = ctx.countries.Select()
    .Include("cities")
    .ToList();
```

You can also include multiple relations:

```cpp
ctx.cities.Select().Include({"countries"}).ToList();
```

### JSON and table output

Show as a formatted table:

```cpp
ctx.cities.Select().ShowList();
```

Show as formatted JSON:

```cpp
ctx.cities.Select().ShowJson();
```

Export to JSON:

```cpp
QByteArray json = ctx.cities.Select().ToJson();
qDebug().noquote() << json;
```

Read the last JSON result:

```cpp
ctx.cities.Select({"id", "name"}).ToList();
QJsonArray json = ctx.cities.GetLastJson();
```

## Query API reference

These are the main query methods available in `Q1Query<T>`:

| Method | Purpose |
| --- | --- |
| `Select(columns)` | Start a query and optionally choose columns |
| `Where(condition)` | Add a `WHERE` clause |
| `Distinct()` | Apply `DISTINCT` for aggregate queries such as `Count("column")` |
| `OrderBy(clause)` | Use a custom `ORDER BY` clause |
| `OrderByAsc(column)` | Sort ascending |
| `OrderByDesc(column)` | Sort descending |
| `Limit(count)` | Limit returned rows |
| `InnerJoin(table, on)` | Add an inner join |
| `LeftJoin(table, on)` | Add a left join |
| `RightJoin(table, on)` | Add a right join |
| `FullJoin(table, on)` | Add a full outer join |
| `GroupBy(columns)` | Add `GROUP BY` |
| `Having(condition)` | Add `HAVING` |
| `Include(relation)` | Eager-load a relation |
| `Count(column)` | Run `COUNT(...)` |
| `Max<T>(column)` | Run `MAX(...)` |
| `Min<T>(column)` | Run `MIN(...)` |
| `Sum<T>(column)` | Run `SUM(...)` |
| `Avg<T>(column)` | Run `AVG(...)` |
| `ToList()` | Execute and return typed entities |
| `ToJson()` | Execute and return JSON bytes |
| `ShowList()` | Execute and print a table |
| `ShowJson()` | Execute and print JSON |

## Important notes

- Call `Initialize()` before CRUD or queries
- `Update()` and `Delete()` require a non-empty `WHERE` clause
- For joins, prefer aliased columns such as `cities.name AS city_name` to avoid column name conflicts
- Joined columns are best inspected through `GetLastJson()` when they do not map directly to the entity type
- `ShowList()` flattens included relation data for easy console output
- `ShowJson()` keeps included relation data nested

## Use Q1ORM in another CMake project

### Option 1: build from source with `add_subdirectory`

```cmake
add_subdirectory(path/to/Q1ORM)

target_link_libraries(MyApp PRIVATE Src Qt6::Core Qt6::Sql)
target_include_directories(MyApp PRIVATE path/to/Q1ORM/src)
```

Note: the CMake target name is `Src`, while the produced DLL/library output name is `Q1ORM`.

### Option 2: use the installed release output

The examples use this pattern:

```cmake
set(Q1ORM_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../../Releases/Release-0.1")

add_library(Q1ORM SHARED IMPORTED)
set_property(TARGET Q1ORM PROPERTY IMPORTED_LOCATION "${Q1ORM_PATH}/bin/Q1ORM.dll")
set_property(TARGET Q1ORM PROPERTY IMPORTED_IMPLIB "${Q1ORM_PATH}/lib/Q1ORM.lib")
target_include_directories(Q1ORM INTERFACE "${Q1ORM_PATH}/include")

target_link_libraries(MyApp PRIVATE Q1ORM Qt6::Core Qt6::Sql)
```

## More documentation

- `Docs/GettingStarted.md`
- `Docs/CRUDGuide.md`
- `Examples/SoloExample/`
- `Examples/UnitTestExample/`

## License

See `LICENSE`.
