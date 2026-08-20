# Q1ORM Project Memory

> این فایل snapshot معماری پروژه برای استفاده در گفت‌وگوهای بعدی است.  
> آخرین بررسی: **2026-08-20**  
> مسیر پروژه: `C:\Users\ARM\Desktop\Q1ORM`

## هدف پروژه

Q1ORM یک ORM مبتنی بر C++/Qt برای کار با دیتابیس است. مدل‌ها به‌صورت کلاس‌های سادهٔ C++ تعریف می‌شوند، با `Q1Entity<T>` به جدول/ستون نگاشت می‌شوند و از طریق `Q1Context`، migration خودکار، CRUD و query fluent انجام می‌شود.

کتابخانهٔ اصلی در `src/` ساخته می‌شود و با نام خروجی `Q1ORM` (DLL/shared library) نصب می‌گردد.

## فناوری و build

- زبان: C++؛ استاندارد واقعی build فعلی در `src/CMakeLists.txt` برابر **C++20** است.
- فریم‌ورک: Qt 6 با ماژول‌های `Core` و `Sql`؛ CMake فعلی صراحتاً Qt6 را پیدا می‌کند.
- build system: CMake، حداقل نسخهٔ اعلام‌شده `3.14`.
- کامپایلر مورد انتظار در اسکریپت انتشار ویندوز: MSVC x64.
- ریشهٔ CMake:
  - `src/` کتابخانه
  - `Examples/` مثال‌ها و تست‌ها
  - `Docs/` مستندات
  - `Tools/` ابزارهای کمکی
- نصب پیش‌فرض کتابخانه در `Releases/Release-0.1/` انجام می‌شود:
  - `bin/` فایل DLL و executableها
  - `lib/` فایل import/static library
  - `include/` هدرهای عمومی
  - `scripts/` اسکریپت نصب PostgreSQL

دستور معمول:

```bash
cmake -S . -B build
cmake --build build
cmake --install build
```

اسکریپت‌های release:

- `release.bat`: فرایند کامل ویندوز، تشخیص Qt/MSVC، build کتابخانه/ابزار/مثال‌ها و install.
- `release.sh`: فرایند سادهٔ لینوکس/Unix برای configure، build و install.

## ساختار مهم فایل‌ها

```text
src/
  Q1ORM.h/.cpp                 facade عمومی کتابخانه
  Q1ORM_global.h               export macro
  Q1Core/
    Q1Context/
      Q1Connection.h           اتصال Qt SQL و تنظیمات backend
      Q1Context.h/.cpp         lifecycle و schema initialization
    Q1Entity/
      Q1Entity.h               template اصلی repository/entity + CRUD
      Q1Column.h/.cpp          metadata ستون و type mapping
      Q1Table.h                تعریف جدول
      Q1Relation.h             تعریف relation/foreign key
    Q1Migration/
      Q1Migration.h/.cpp       اجرای migration روی دیتابیس
      Q1MigrationQuery.h/.cpp  تولید SQL مخصوص backend
    Q1Query/Q1Query.h          query builder fluent و خروجی JSON/table
  Q1DatabaseInstall/
    Q1DatabaseInstall.h/.cpp   نصب PostgreSQL در ویندوز با batch/QProcess

Examples/
  SoloExample/                 مثال ساده PostgreSQL/SQL Server، دو جدول
  SqliteTestExample/           مثال چندجدولی با SQLite و قابل تغییر به backendهای دیگر
  MySqlTestExample/            مثال چندجدولی MySQL/SQL Server
  DatabaseInstallExample/      نمونهٔ استفاده از Q1DatabaseInstall
  UnitTestExample/             تست‌های Qt Test و SQL generation

Docs/
  GettingStarted.md
  CRUDGuide.md

Tools/
  ExampleProjectModifier/      ابزار ساخت/تغییر پروژهٔ نمونه از template
  ReleaseInstaller/            ابزار release installer
  Template/                    template سادهٔ پروژهٔ مصرف‌کننده
```

دایرکتوری‌های `build/`، فایل‌های داخل `Releases/` و DLL/EXEها خروجی build یا release هستند و منبع اصلی منطق محسوب نمی‌شوند؛ برای فهم رفتار باید ابتدا `src/` و مثال‌ها خوانده شوند.

## لایهٔ اتصال: `Q1Connection`

فایل: `src/Q1Core/Q1Context/Q1Connection.h`

```cpp
enum Q1Driver {
    POSTGRE_SQL,
    SQLSERVER,
    MYSQL,
    SQLITE
};
```

رفتارهای مهم:

- از `QSqlDatabase::addDatabase` دو اتصال نام‌دار می‌سازد:
  - اتصال عادی با نام یکتا مانند `conn_<uuid>`
  - اتصال root با نام `root-<name>` برای ساخت database
- driverهای Qt:
  - PostgreSQL: `QPSQL`
  - SQL Server: `QODBC`
  - MySQL: `QMYSQL`
  - SQLite: `QSQLITE`
- پورت پیش‌فرض: PostgreSQL `5432`، SQL Server `1433`، MySQL `3306`، SQLite `0`.
- برای SQL Server، مقدار host می‌تواند host ساده، DSN یا connection string دارای `Driver=`/`DSN=` باشد.
- اگر connection string کامل نباشد، driver ODBC از `Q1ORM_SQLSERVER_ODBC_DRIVER` خوانده می‌شود و مقدار پیش‌فرض `ODBC Driver 17 for SQL Server` است.
- `Connect()` و `RootConnect()` اتصال را باز می‌کنند؛ `Disconnect()` و `RootDisconnect()` می‌بندند.
- متدهای `QuoteIdentifier` و `QuoteStringLiteral` quoting مناسب SQL Server/MySQL/سایر backendها را فراهم می‌کنند.
- خطای آخر در `error` و `error_type` نگهداری می‌شود و با `ErrorMessage()` خوانده می‌شود.

## context و lifecycle

فایل‌های `Q1Context.h/.cpp`

کلاس مصرف‌کننده باید از `Q1Context` مشتق شود و این سه متد را override کند:

```cpp
virtual void OnConfiguration() = 0;
virtual QList<Q1Table*> OnTablesCreating() = 0;
virtual QList<Q1Relation> OnTableRelationCreating() = 0;
```

الگوی اجرای `Initialize()`:

1. اجرای `OnConfiguration()`
2. ساخت `Q1Migration`
3. اتصال به root database و ساخت database هدف در صورت نبودن
4. اتصال به database هدف
5. اجرای `OnTablesCreating()` برای configure کردن entityها
6. ساخت جدول‌های مفقود
7. مقایسهٔ schema ستون‌ها:
   - حذف ستون‌هایی که دیگر declare نشده‌اند
   - افزودن ستون‌های جدید
   - تغییر اندازه
   - تغییر nullable در صورت امن بودن
   - تغییر default
8. اجرای `OnTableRelationCreating()` و ساخت foreign keyها

`SetConnection(Q1Connection*, bool takeOwnership)` تعیین می‌کند context مالک connection هست یا نه. در destructor، query آزاد می‌شود، اتصال بسته می‌شود و فقط در حالت ownership connection حذف می‌شود.

`GetLastError()` ابتدا خطای migration و سپس خطای connection را برمی‌گرداند.

## مدل‌سازی entity و metadata

فایل: `src/Q1Core/Q1Entity/Q1Entity.h`

`Q1Entity<Entity>` از خود `Entity` ارث می‌برد و نقش repository برای همان مدل را دارد. سازنده در صورت وجود متدهای static زیر آن‌ها را خودکار صدا می‌زند:

```cpp
static void ConfigureEntity(Q1Entity<T>& entity);
static QList<Q1Relation> CreateRelations(Q1Entity<T>& entity);
```

الگوی معمول mapping:

```cpp
entity.ToTableName("countries");
entity.Property(entity.id, "id", false, true, "GENERATED ALWAYS AS IDENTITY");
entity.Property(entity.name, "name", false, false);
entity.Relations("countries", "cities", ONE_TO_MANY, "country_id", "id");
```

`Property(...)`:

- نام ستون، nullable، primary key و default را ثبت می‌کند.
- نوع C++ را با `typeid(Member).name()` به `Q1ColumnDataType` تبدیل می‌کند.
- برای `VARCHAR`/`CHAR` اندازهٔ پیش‌فرض `255` می‌گذارد.
- offset عضو را داخل `PropertyInfo` نگه می‌دارد و CRUD با pointer arithmetic مقدار عضو را می‌خواند/می‌نویسد.

نوع‌های پشتیبانی‌شدهٔ metadata:

`INTEGER`, `SMALLINT`, `BIGINT`, `REAL`, `DOUBLE_PRECISION`, `BOOLEAN`, `CHAR`, `TEXT`, `VARCHAR`, `DATE`, `TIMESTAMP`

متدهای مهم `Q1Entity`:

- schema: `GetTable()`, `GetTablePtr()`, `GetRelations()`, `GetTableColumns()`
- CRUD: `Insert`, `Select`, `Update`, `UpdateById`, `Delete`, `DeleteById`
- کمکی: `CheckTableSchema`, `EntityToJson`, `ExecuteScalar`, `ExecuteQuery`, `ExecuteRelationQuery`
- خروجی آخر: `GetLastJson()`, `SetLastJson()`
- خطا: `GetLastError()`

در `Insert`، primary key خودکار از INSERT کنار گذاشته می‌شود و برای backendها از روش مناسب گرفتن id استفاده می‌شود:

- PostgreSQL: `RETURNING`
- SQL Server: `OUTPUT INSERTED`
- MySQL/SQLite: سپس خواندن id تولیدشده

## جدول، ستون و relation

### `Q1Column`

در `src/Q1Core/Q1Entity/Q1Column.h` قرار دارد. برابری ستون‌ها بر اساس نام case-insensitive است. متدهای static برای تبدیل نوع C++/نوع دیتابیس، تشخیص identity، normalize کردن default و مقایسهٔ defaultها دارد.

### `Q1Table`

در `Q1Table.h`:

- `table_name`
- `QList<Q1Column> columns`
- `QList<Q1Relation> relations`

متدهای کاربردی: `FindColumn`, `HasColumn`, `GetPrimaryKeys`, `IsValid`, `Clear`.

### `Q1Relation`

در `Q1Relation.h`:

```cpp
enum Q1RelationType {
    ONE_TO_ONE,
    ONE_TO_MANY,
    MANY_TO_ONE,
    MANY_TO_MANY
};
```

اطلاعات relation:

- `base_table`: جدول دارای foreign key
- `top_table`: جدول مرجع/مرتبط
- `foreign_key`
- `reference_key`
- `on_delete` و `on_update`
- `lazy_load` و `back_reference`

نام constraint با الگوی زیر تولید می‌شود:

```text
FK_<base_table>_<top_table>_<foreign_key>
```

در mappingهای موجود، relationها از هر دو سمت توصیف می‌شوند، اما `OnTableRelationCreating()` معمولاً فقط relationهای لازم برای ساخت constraint را برمی‌گرداند تا duplicate ایجاد نشود.

## migration و SQL dialect

`Q1Migration` عملیات runtime را انجام می‌دهد و `Q1MigrationQuery` فقط SQL مخصوص backend را تولید می‌کند. backendهای translator:

- PostgreSQL
- SQL Server
- MySQL
- SQLite

عملیات شامل:

- فهرست databaseها، tableها و columnها
- ساخت database/table/column
- حذف table/column
- تغییر nullable، default و اندازه
- تشخیص null data
- ساخت و بررسی constraint

نکتهٔ backendی مهم:

- SQLite برای افزودن relation محدودیت `ALTER TABLE` دارد؛ کد در `BuildSqliteAddRelationSQL` جدول را با schema فعلی بازسازی می‌کند، داده را منتقل می‌کند، جدول قدیمی را حذف و جدول موقت را rename می‌کند.
- ساخت database برای SQLite عملاً لازم نیست؛ مسیر فایل به‌عنوان database name استفاده می‌شود.
- SQL Server برای تغییر nullable/default از metadata و constraintهای سیستم استفاده می‌کند.
- relation نوع `MANY_TO_MANY` جدول junction تولید می‌کند.

## query builder

فایل: `src/Q1Core/Q1Query/Q1Query.h`

`Q1Query<Entity>` به‌صورت fluent روی `Q1Entity` کار می‌کند.

عملیات:

```cpp
Select({"cities.name AS city", "countries.name AS country"})
Where("cities.id > 1")
OrderByAsc("cities.name")
OrderByDesc("cities.name")
OrderBy("id DESC")
Limit(10)
Distinct()
InnerJoin(...)
LeftJoin(...)
RightJoin(...)
FullJoin(...)
GroupBy(...)
Having(...)
Include("countries")
```

اجرای نهایی:

- `ToList()` → `QList<Entity>`
- `ToJson()` → `QByteArray`
- `ShowList()` → نمایش جدولی با `TableDebugger`
- `ShowJson()` → چاپ JSON

Aggregateها:

`Count`, `Max`, `Min`, `Sum`, `Avg`

نتیجهٔ query علاوه بر entityهای typed در `Q1Entity::lastJson` ذخیره می‌شود؛ بنابراین در joinها، aliasها و ستون‌های غیرمدل باید از `GetLastJson()` استفاده کرد.

`Include()` eager-loading را با query جداگانه انجام می‌دهد و دادهٔ relation را به JSON اضافه می‌کند. در `ShowList()`، دادهٔ include ممکن است با prefixهایی مانند `countries_name` flatten شود.

هشدار عملی: مقدارهای `Where`, `OrderBy`, join condition و column expression به‌صورت رشتهٔ SQL دریافت می‌شوند؛ caller مسئول درست بودن SQL و جلوگیری از injection در این لایه است.

## مثال‌ها

### `SoloExample`

دو مدل `Country` و `City` دارد:

```text
countries (id, name)
cities    (id, name, country_id)
```

تنظیمات پیش‌فرض environment:

- `Q1ORM_DB_DRIVER=postgres`
- host `localhost`
- database `q1orm_test`
- user `postgres`
- password `123`
- port بر اساس driver

`sqlserver`, `mssql` و `odbc` در driver به SQL Server تبدیل می‌شوند؛ هر مقدار دیگرِ پیش‌فرض PostgreSQL است.

### `SqliteTestExample`

پنج مدل/جدول دارد:

```text
countries -> cities -> clubs -> courts -> users
```

کلیدهای خارجی:

- `cities.country_id -> countries.id`
- `clubs.city_id -> cities.id`
- `courts.club_id -> clubs.id`
- `users.court_id -> courts.id`

تنظیمات پیش‌فرض:

- driver: SQLite
- مسیر فایل: `sqlite_test.db`
- قابل override با `Q1ORM_DB_PATH`
- برای غیر-SQLite نیز از `Q1ORM_DB_DRIVER`, `Q1ORM_DB_HOST`, `Q1ORM_DB_NAME`, `Q1ORM_DB_USER`, `Q1ORM_DB_PASSWORD`, `Q1ORM_DB_PORT` استفاده می‌کند.

### `MySqlTestExample`

همان زنجیرهٔ پنج‌جدولی را با driver پیش‌فرض MySQL و database پیش‌فرض `DoctorPadelDb` اجرا می‌کند. تشخیص `sqlserver`/`mssql`/`odbc` به SQL Server و در غیر این صورت به MySQL می‌رود.

### `DatabaseInstallExample`

با `Q1DatabaseInstall` یک batch می‌سازد و از `cmd.exe`/PowerShell برای دانلود، extract، initdb و register کردن PostgreSQL Windows service استفاده می‌کند. این بخش Windows-specific است.

### `UnitTestExample`

شامل:

- تست‌های integration برای select/where/order/limit/aggregate/join/group/include/JSON/CRUD
- تست مستقل تولید SQL در `SqlGenerationTests`
- backendهای PostgreSQL و SQL Server با environment variable
- اگر Qt SQL driver یا database در دسترس نباشد، تست integration با `QSKIP` رد می‌شود.

environmentهای تست:

- PostgreSQL: `Q1ORM_PG_HOST`, `Q1ORM_PG_DB_NAME`, `Q1ORM_PG_USER`, `Q1ORM_PG_PASSWORD`, `Q1ORM_PG_PORT`
- SQL Server: `Q1ORM_SQLSERVER_HOST`, `Q1ORM_SQLSERVER_DB_NAME`, `Q1ORM_SQLSERVER_USER`, `Q1ORM_SQLSERVER_PASSWORD`, `Q1ORM_SQLSERVER_PORT`
- fallback مشترک: `Q1ORM_DB_HOST`, `Q1ORM_DB_USER`, `Q1ORM_DB_PASSWORD`, `Q1ORM_DB_PORT`, `Q1ORM_TEST_DB_NAME`

## ابزارها

### ExampleProjectModifier / EPModifier

برنامهٔ Qt Core برای ساخت پروژهٔ نمونه از templateهای `Tools/ExampleProjectModifier/Template` یا `Tools/Template`. template نام پروژه را با placeholder `{ExampleProjectName}` می‌گیرد و به release نصب‌شدهٔ Q1ORM لینک می‌شود.

### ReleaseInstaller

برنامهٔ Qt Core مستقل برای عملیات نصب release. جزئیات رفتار اجرایی در `Tools/ReleaseInstaller/main.cpp` است.

## مستندات موجود

- `README.md`: معرفی، build، env، quick start و API نمونه
- `Docs/GettingStarted.md`: مفاهیم، build و شروع کار
- `Docs/CRUDGuide.md`: مسیر کامل model → mapping → context → initialize → CRUD/query

در صورت اختلاف جزئی بین متن مستندات، منبع تصمیم‌گیری باید کد و CMake فعلی باشد؛ به‌خصوص:

- `src/CMakeLists.txt` استاندارد C++20 را تعیین می‌کند.
- بعضی متن‌های قدیمی هنوز C++17 را ذکر می‌کنند.

## قرارداد پاسخ‌گویی آینده

برای سؤال‌های بعدی دربارهٔ این پروژه:

1. ابتدا این فایل را به‌عنوان نقشهٔ معماری بخوان.
2. اگر سؤال دربارهٔ behavior یا bug در بخشی باشد که این فایل جزئیات کافی ندارد، فقط همان فایل/زیرسیستم را بررسی کن، نه کل پروژه.
3. برای تغییر API یا schema، ارتباط بین `Q1Context` → `Q1Migration` → `Q1MigrationQuery` → `Q1Entity/Q1Query` را در نظر بگیر.
4. برای مثال‌های اجرا، تفاوت connection configuration و mapping بین `SoloExample`, `SqliteTestExample` و `MySqlTestExample` را حفظ کن.
5. خروجی‌های `build/` و `Releases/` را با source اشتباه نگیر؛ آن‌ها ممکن است stale یا حاصل تغییرات محلی باشند.

## وضعیت کاری هنگام تهیهٔ این فایل

در زمان تهیهٔ این snapshot، working tree از قبل تغییرات و فایل‌های تولیدی متعدد داشت، از جمله تغییرات در CMake، هدرهای core، migrationها، مثال SQLite، release binaryها و محتوای `build/`. این فایل به‌صورت مستقل اضافه شده و آن تغییرات را اصلاح، commit، reset یا حذف نکرده است.

## نقاطی که هنگام تغییر باید با احتیاط بررسی شوند

- `Q1Entity` به offset اعضای مدل و layout کلاس وابسته است؛ تغییر مدل یا ارث‌بری می‌تواند روی mapping اثر بگذارد.
- تشخیص identity و default بین backendها حساس است.
- SQLite relation migration با بازسازی جدول انجام می‌شود و باید foreign keyها/داده‌ها بررسی شوند.
- نام‌گذاری relationها و جهت `ONE_TO_MANY`/`MANY_TO_ONE` در translator مهم است.
- query builder رشتهٔ خام SQL می‌گیرد و مسئولیت quoting/validation کامل با caller نیست.
- `Q1Connection` از connectionهای named در Qt استفاده می‌کند؛ lifecycle و حذف connection باید با دقت انجام شود.
- در joinها، entity typed فقط ستون‌های مدل اصلی را پر می‌کند؛ ستون‌های joined/alias شده در `GetLastJson()` قابل اتکا هستند.
