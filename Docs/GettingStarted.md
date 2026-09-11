# Q1ORM Getting Started

This document gives a quick overview of the project, its structure, and the main steps required to build and try the examples.

## Project layout

- `src/` contains the Q1ORM library source code.
- `Examples/` contains sample applications and tests that use the library.
- `Docs/` stores project documentation.
- `Releases/` contains built release output that can be consumed by example projects.
- `Tools/` contains helper tools used by the project.

## Core concepts

Q1ORM exposes a small set of Qt-friendly building blocks for database work:

- `Q1Connection` manages the database connection details.
- `Q1Context` is the base class for an application database context.
- `Q1Entity<T>` represents a table-like entity set in the context.
- `Q1Query` provides query-building operations such as select, where, joins, grouping, ordering, and aggregates.
- `Q1DatabaseInstall` provides helper functionality for PostgreSQL installation.

## Build requirements

Before building the project, make sure these tools are available:

- CMake 3.14 or newer
- Qt 6 with the `Core` and `Sql` modules
- A C++20 compatible compiler
- The SQLite Qt SQL driver for `DbExample`

## Build the project

An example build flow with CMake:

```bash
cmake -S . -B build
cmake --build build
```

If you want to install the built library into the release folder layout used by the examples:

```bash
cmake --install build
```

## Database configuration

The examples support both PostgreSQL and SQL Server through `Q1Connection`.

Use these variables to select and configure the backend:

- `Q1ORM_DB_DRIVER=postgres` or `Q1ORM_DB_DRIVER=sqlserver`
- `Q1ORM_PG_HOST`
- `Q1ORM_PG_DB_NAME`
- `Q1ORM_PG_USER`
- `Q1ORM_PG_PASSWORD`
- `Q1ORM_PG_PORT`
- `Q1ORM_SQLSERVER_HOST`
- `Q1ORM_SQLSERVER_DB_NAME`
- `Q1ORM_SQLSERVER_USER`
- `Q1ORM_SQLSERVER_PASSWORD`
- `Q1ORM_SQLSERVER_PORT`
- `Q1ORM_SQLSERVER_ODBC_DRIVER`

## Creating a context

Include `<Q1ORM.h>` to access the context, entity sets, and model builder.
Configure each entity with a map, then apply the map to a context member:

```cpp
struct City {
    int id = 0;
    QString name;
};

struct CityMap {
    static void ConfigureEntity(Q1Entity<City>& entity) {
        entity.ToTableName("cities");
        entity.HasKey<&City::id>().ValueGeneratedOnAdd();
        entity.Property<&City::name>().IsRequired();
    }
};

class ApplicationDbContext : public Q1Context {
public:
    explicit ApplicationDbContext(Q1Connection* connection) {
        SetConnection(connection);
    }

    Q1Entity<City> cities;

protected:
    void OnModelCreating(Q1ModelBuilder& builder) override {
        builder.ApplyMap<CityMap>(cities);
    }
};
```

`ApplyMap<Map>(member)` registers and configures the member automatically.
Calls can be chained. Configure relationships inside `ConfigureEntity` with
`HasOne<Principal>().WithMany().HasForeignKey<&Entity::foreign_key>()`
followed by `.HasPrincipalKey<&Principal::id>()`. Relationships resolve after all
maps have configured their tables.

The no-argument form `builder.ApplyMap<CityMap>()` is also supported: first call
`RegisterEntity(&cities)` in the context constructor. The entity type is inferred
from the map's public static `ConfigureEntity(Q1Entity<City>&)` method. Neither
form requires an `EntityType` alias or inheritance from `Q1EntityMap`.

Call `Initialize()` before using the entity sets and check its boolean result.
Mapping errors are available through `GetLastError()`. The connection and entity
members must outlive their use by the context; `SetConnection(connection)` borrows
the connection by default.

See `Examples/DbExample/main.cpp` for a runnable SQLite example with two maps.

## Query examples

The examples in the repository demonstrate patterns such as:

- selecting all rows or specific columns
- filtering with `Where(...)`
- ordering with `OrderByAsc(...)` and `OrderByDesc(...)`
- limiting rows with `Limit(...)`
- aggregate operations like `Count()`, `Max()`, `Min()`, `Sum()`, and `Avg()`
- joins such as inner, left, right, and full joins
- grouping with `GroupBy(...)` and `Having(...)`

## Example folders

- `Examples/UnitTestExample/` contains the complete integration and SQL generation tests.
- `Examples/DockerTestExample/` runs the same tests against PostgreSQL in Docker.

## Notes

- `Q1Context::Initialize()` is the main entry point for preparing the context.
- `Q1Context::GetLastError()` can be used to inspect connection or query failures.
- The PostgreSQL installer helper writes and runs a batch script, so it is primarily intended for Windows environments.

## Performance and safety APIs

Q1ORM supports parameterized predicates and explicit indexes:

```cpp
entity.Index({"email"}, true);
entity.Index({"tenant_id", "created_at"});

Q1Query<User> query(&users);
query.Where("age", Q1Operator::GreaterThan, 18)
     .WhereEqual("status", "active")
     .OrderBy("created_at", Q1Sort::Descending)
     .Take(50);
```

`Q1Context::Initialize()` creates declared indexes and indexes foreign-key columns. Use the RAII transaction guard for exception-safe transactions:

```cpp
auto transaction = connection.Transaction();
// writes...
transaction.Commit();
```

`Any()` and `First()` avoid materializing more rows than needed.

## Typed mapping

Use member pointers to map properties without repeating their column names:

```cpp
class CityMap {
public:
    static void ConfigureEntity(Q1Entity<City>& entity) {
        entity.ToTableName("cities"); // Optional: otherwise the table is named City.
        entity.HasKey<&City::id>().ValueGeneratedOnAdd();
        entity.Property<&City::name>().IsRequired();
        entity.Property<&City::country_id>();
        entity.HasOne<Country>().WithMany()
            .HasForeignKey<&City::country_id>()
            .HasPrincipalKey<&Country::id>();
    }
};
```

Apply both `CityMap` and `CountryMap` to the model. `CountryMap` must map
`Country::id`. Map order does not matter: relationships resolve after all maps
have configured their tables. Properties and relationships are both declared
inside `ConfigureEntity`; an inverse declaration is unnecessary.
Both key members must have matching C++ types and be mapped. Missing principal
entities or key mappings cause `Build()` / `Initialize()` to fail.

`Property<&City::name>()` defaults to a required column named `name`.
Use `.IsRequired(false)` for nullable database columns, or
`.HasColumnName("display_name")` for a custom name. Typed relationships use
these configured names, including overrides on either key. Empty or conflicting
column overrides throw `std::invalid_argument`.
`HasKey` marks a nonnullable primary key; `ValueGeneratedOnAdd()` selects identity
generation for integer keys without embedding SQL in the map.

Omit `ToTableName` for the unqualified C++ entity name (`City`, not `cities`).
No automatic English pluralization is performed. Automatic member and type
names use compiler signatures on GCC, Clang, and MSVC, rather than C# expression
trees. This is compiler-specific C++20 name extraction; the existing explicit
string-based API remains available. This API configures schema relationships;
it does not add navigation-property loading or C# lambda expressions.
