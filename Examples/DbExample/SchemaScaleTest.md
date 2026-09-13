# 300-table schema test

Build and run from the repository root:

```sh
cmake --build build-ubuntu --target SchemaScaleTest -j 4
ctest --test-dir build-ubuntu -R Q1ORM_SchemaScale300 -V
```

Use your own CMake build directory if it differs from `build-ubuntu`.
The test lives alongside DbExample as a separate executable. It uses a temporary,
file-backed SQLite database and deletes that database when it finishes.

The test creates 300 mapped tables through `Q1Context::Initialize()`, with 599
indexes and 299 foreign keys pointing to the first table. It inserts 100 rows
per table through the ORM (30,000 total, in one transaction), closes and reopens
the connection, then initializes a fresh context against the populated database.
It next adds a required `revision` column with default `7` to all 300 maps and
runs initialization again, followed by another unchanged startup.

After each phase it checks every table's column names/count, index names/count,
foreign-key target/columns, and every saved row's values. It also checks SQLite
integrity and foreign-key validity, confirms foreign-key enforcement is enabled,
and confirms that no migration-history table was created. Unchanged startups
must leave SQLite's schema version unchanged.

The output reports initialization and insertion timings separately; validation
is outside those timings. Results describe this local SQLite workload with a
single process, 100 rows per table, and a shared model type explicitly mapped to
300 distinct table names. This does not benchmark 300 distinct C++ types, remote
databases, millions of rows, concurrent application instances, or online updates
under production traffic. There are no strict timing assertions; CTest has a
120-second timeout to catch hangs.
