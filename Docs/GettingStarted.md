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
- Qt 5 or Qt 6 with the `Core` module
- A C++17 compatible compiler
- PostgreSQL or SQL Server access for database-backed examples

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

Typical usage is to derive an application-specific context from `Q1Context`, then expose entity sets as members:

```cpp
class ApplicationDbContext : public Q1Context
{
public:
    explicit ApplicationDbContext(Q1Connection* conn);

    void OnConfiguration() override;
    QList<Q1Table*> OnTablesCreating() override;
    QList<Q1Relation> OnTableRelationCreating() override;

    Q1Entity<City> cities;
    Q1Entity<Country> countries;
};
```

In the derived context, configure the connection, declare the tables, and describe any relations needed by the model.

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

- `Examples/SoloExample/` is a lightweight usage example.
- `Examples/DatabaseInstallExample/` demonstrates database installation support.
- `Examples/UnitTestExample/` contains integration and SQL generation tests.

## Notes

- `Q1Context::Initialize()` is the main entry point for preparing the context.
- `Q1Context::GetLastError()` can be used to inspect connection or query failures.
- The PostgreSQL installer helper writes and runs a batch script, so it is primarily intended for Windows environments.
