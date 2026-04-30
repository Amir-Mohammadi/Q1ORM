# Q1ORM CRUD Guide

This guide shows the normal Q1ORM flow:

1. define your model classes
2. map them to tables
3. create a `Q1Context`
4. connect the database
5. call `Initialize()`
6. use `Insert`, `Select`, `Update`, and `Delete`

The examples below match the APIs used in this project.

## 1. Create model classes

Example models:

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

## 2. Map models to database tables

Each model should be configured with table and column metadata.

### Country mapping

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

### City mapping

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

## 3. Create a DbContext

Your application context should inherit from `Q1Context`.

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

Implementation:

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

## 4. Connect to the database

Create a `Q1Connection` first, then pass it to your context.

### PostgreSQL

```cpp
Q1Connection* conn = new Q1Connection(
    Q1Driver::POSTGRE_SQL,
    "localhost",
    "q1orm_test",
    "postgres",
    "123",
    5432
);
```

### SQL Server

```cpp
Q1Connection* conn = new Q1Connection(
    Q1Driver::SQLSERVER,
    "localhost",
    "q1orm_test",
    "sa",
    "your_password",
    1433
);
```

`Q1Connection` handles opening and closing internally when CRUD methods run.

## 5. Initialize the DbContext

After creating the connection and context, call `Initialize()`.

```cpp
ApplicationDbContext ctx(conn);

if (!ctx.Initialize())
{
    qWarning() << "Initialization failed:" << ctx.GetLastError();
    return -1;
}
```

What `Initialize()` does:

- configures the connection
- creates the database if needed
- creates missing tables
- adds missing columns
- creates relations

That means you usually call `Initialize()` once at application startup before CRUD.

## 6. Create data

Use `Insert()` to save a new row.

### Insert a country

```cpp
Country usa;
usa.name = "USA";

if (!ctx.countries.Insert(usa))
{
    qWarning() << ctx.countries.GetLastError();
}

qDebug() << "New country id:" << usa.id;
```

If the primary key is identity/auto-increment, Q1ORM writes the generated id back into the object after insert.

### Insert a city

```cpp
City newYork;
newYork.name = "New York";
newYork.country_id = usa.id;

if (!ctx.cities.Insert(newYork))
{
    qWarning() << ctx.cities.GetLastError();
}
```

## 7. Read data

Q1ORM uses `Select()` and then query builder methods like `Where`, `OrderByAsc`, `Limit`, and `ToList`.

### Select all rows

```cpp
QList<Country> countries = ctx.countries.Select().ToList();
QList<City> cities = ctx.cities.Select().ToList();
```

### Select specific columns

```cpp
QList<City> cities = ctx.cities.Select({"id", "name"}).ToList();
```

### Filter rows

```cpp
QList<City> usaCities = ctx.cities.Select()
    .Where(QString("country_id = %1").arg(usa.id))
    .ToList();
```

### Sort rows

```cpp
QList<City> orderedCities = ctx.cities.Select()
    .OrderByAsc("name")
    .ToList();
```

### Limit rows

```cpp
QList<City> firstTwoCities = ctx.cities.Select()
    .OrderByAsc("name")
    .Limit(2)
    .ToList();
```

### Get one row

There is no dedicated `First()` helper here, so use `ToList()` and check the first item yourself.

```cpp
QList<Country> result = ctx.countries.Select()
    .Where("id = 1")
    .ToList();

if (!result.isEmpty())
{
    Country country = result.first();
    qDebug() << country.name;
}
```

### Count / Max / Min / Sum / Avg

```cpp
int totalCities = ctx.cities.Select().Count();
int maxId = ctx.cities.Select().Max<int>("id");
int minId = ctx.cities.Select().Min<int>("id");
int sumIds = ctx.cities.Select().Sum<int>("id");
double avgId = ctx.cities.Select().Avg<double>("id");
```

### Join tables

```cpp
ctx.cities.Select({"cities.name AS city", "countries.name AS country"})
    .InnerJoin("countries", "cities.country_id = countries.id")
    .OrderByAsc("cities.name")
    .ToList();
```

Joined or aliased columns are available in `GetLastJson()`.

```cpp
QJsonArray json = ctx.cities.GetLastJson();
```

### Include related data

```cpp
QList<Country> countriesWithCities = ctx.countries.Select()
    .Include("cities")
    .ToList();
```

This keeps the main typed list and also stores relation data in the last JSON result.

## 8. Show query results

You can display query output directly for debugging or demos.

### Show as table

```cpp
ctx.cities.Select().ShowList();
```

### Show as JSON

```cpp
ctx.cities.Select().ShowJson();
```

### Export as JSON bytes

```cpp
QByteArray json = ctx.cities.Select().ToJson();
qDebug().noquote() << json;
```

### Access last JSON result

```cpp
ctx.cities.Select({"id", "name"}).ToList();
QJsonArray json = ctx.cities.GetLastJson();
```

## 9. Update data

Use `Update()` or `UpdateById()`.

### Update with id

```cpp
Country country = ctx.countries.Select()
    .Where("id = 1")
    .ToList()
    .first();

country.name = "United States";

if (!ctx.countries.UpdateById(country, country.id))
{
    qWarning() << ctx.countries.GetLastError();
}
```

### Update with custom condition

```cpp
City city = ctx.cities.Select()
    .Where("id = 1")
    .ToList()
    .first();

city.name = "New York City";

if (!ctx.cities.Update(city, "id = 1"))
{
    qWarning() << ctx.cities.GetLastError();
}
```

Important notes:

- `Update()` does not allow an empty `WHERE` clause.
- primary key columns are skipped during update.

## 10. Delete data

Use `Delete()` or `DeleteById()`.

### Delete by id

```cpp
if (!ctx.cities.DeleteById(1))
{
    qWarning() << ctx.cities.GetLastError();
}
```

### Delete with custom condition

```cpp
if (!ctx.cities.Delete("country_id = 2"))
{
    qWarning() << ctx.cities.GetLastError();
}
```

Important notes:

- `Delete()` does not allow an empty `WHERE` clause.
- this safety check helps prevent deleting all rows by mistake.

## 11. Full CRUD example

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
    qWarning() << "Init failed:" << ctx.GetLastError();
    return;
}

Country usa;
usa.name = "USA";
ctx.countries.Insert(usa);

City ny;
ny.name = "New York";
ny.country_id = usa.id;
ctx.cities.Insert(ny);

ctx.cities.Select().ShowList();

ny.name = "New York City";
ctx.cities.UpdateById(ny, ny.id);

QList<City> cities = ctx.cities.Select()
    .Where(QString("country_id = %1").arg(usa.id))
    .OrderByAsc("name")
    .ToList();

ctx.cities.DeleteById(ny.id);
ctx.countries.DeleteById(usa.id);
```

## 12. Common mistakes

- calling CRUD before `Initialize()`
- forgetting to set the table name with `ToTableName(...)`
- forgetting to map properties with `Property(...)`
- using `Update()` with an empty `WHERE` clause
- using `Delete()` with an empty `WHERE` clause
- assuming joined columns are copied into the typed entity instead of `GetLastJson()`

## 13. Recommended learning order

If you are new to this project, learn it in this order:

1. `Q1Connection`
2. model class
3. mapping with `Property(...)`
4. `Q1Context`
5. `Initialize()`
6. `Insert()`
7. `Select().ToList()`
8. `UpdateById()`
9. `DeleteById()`
10. joins, includes, and JSON output
