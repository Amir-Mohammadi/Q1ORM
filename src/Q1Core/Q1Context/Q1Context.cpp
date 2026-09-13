#include "Q1Context.h"
#include "../Q1Entity/Q1ModelBuilder.h"
#include <QScopeGuard>
#include <QHash>

Q1Context::~Q1Context()
{
    if (query)
    {
        delete query;
        query = nullptr;
    }

    if (connection)
    {
        if (owns_connection)
        {
            connection->Disconnect();
            connection->RootDisconnect();
            delete connection;
        }
        connection = nullptr;
    }
}

bool Q1Context::Initialize()
{
    model_error.clear();
    if (!connection) {
        qWarning() << "Q1Context::Initialize - connection is null!";
        return false;
    }

    if (connection->InTransaction()) {
        model_error = "Initialize must run outside an application transaction.";
        return false;
    }

    Q1ModelBuilder builder(this);
    OnModelCreating(builder);

    if (!builder.Build()) {
        model_error = builder.Errors().join(QLatin1Char('\n'));
        for (const QString &e : builder.Errors())
            qCritical() << "[Q1ORM] model configuration:" << e;
        return false;
    }

    const QList<Q1Table *> &tables   = builder.Tables();
    const QList<Q1Relation> &relations = builder.Relations();


    database_name = connection->GetDatabaseName();

    delete query;
    query = new Q1Migration(*connection);

    // An existing application database needs no server-level database discovery.
    if (!connection->Connect() && !InitialDatabase()) {
        qCritical() << "Q1Context::Initialize - database initialization failed:"
                    << GetLastError();
        return false;
    }

    const auto cleanup = qScopeGuard([this] {
        query->RollbackSchemaUpdate();
        connection->Disconnect();
    });

    if (!connection->IsOpen()) {
        qCritical() << "Q1Context::Initialize - connection not open after InitialDatabase!";
        return false;
    }

    if (!query->BeginSchemaUpdate()) return false;

    if (!InitialTables(tables)) {
        qCritical() << "Q1Context::Initialize - table initialization failed:"
                    << GetLastError();
        return false;
    }

    if (!InitialColumns(tables)) {
        qCritical() << "Q1Context::Initialize - column initialization failed:"
                    << GetLastError();
        return false;
    }

    if (!InitialRelations(relations)) {
        qCritical() << "Q1Context::Initialize - relation initialization failed:" << GetLastError();
        return false;
    }

    for (Q1Table *table : tables) {
        if (table && !query->EnsureIndexes(*table)) return false;
    }

    if (!query->CommitSchemaUpdate()) return false;

    return true;
}

bool Q1Context::InitialDatabase()
{
    if (!connection || !query) return false;

    if (connection->IsSqlite())
    {
        if (!connection->Connect())
        {
            qCritical() << "Failed to open SQLite database" << database_name
                        << "-" << connection->ErrorMessage();
            return false;
        }
        return true;
    }

    if (!connection->RootConnect())
    {
        qCritical() << "Cannot connect to server:" << connection->ErrorMessage();
        return false;
    }

    QStringList databases = query->GetDatabases();
    if (!databases.contains(database_name))
    {
        qDebug() << "InitialDatabase - database not found. Creating:" << database_name;

        if (!query->AddDatabase(database_name))
        {
            qCritical() << "Failed to create database:" << connection->ErrorMessage();
            connection->RootDisconnect();
            return false;
        }

        qDebug() << "Database created successfully:" << database_name;
    }
    else
    {
        qDebug() << "InitialDatabase - database already exists:" << database_name;
    }

    connection->RootDisconnect();

    if (!connection->Connect())
    {
        qCritical() << "Failed to connect to database" << database_name
                    << "-" << connection->ErrorMessage();
        return false;
    }

    return true;
}

bool Q1Context::InitialTables(const QList<Q1Table*>& tables)
{
    if (!query || !connection) return false;

    QStringList database_tables = query->GetTables();
    if (!query->ErrorMessage().isEmpty()) return false;

    for (Q1Table* table : tables)
    {
        if (!table) continue;

        QString table_name = table->GetName();

        if (!database_tables.contains(table_name))
        {
            qDebug() << "InitialTables - creating table:" << table_name;

            Q1Table q1table;
            q1table.SetName(table_name);

            for (const Q1Column &column : table->GetColumns())
            {
                q1table.columns.append(column);
            }

            if (!query->CreateTableWithColumns(q1table))
            {
                qWarning() << "InitialTables - failed to create table:"
                           << table_name << "-" << query->ErrorMessage();
                return false;
            }
            else
            {
                qDebug() << "InitialTables - table created successfully:" << table_name;
            }
        }
        else
        {
            qDebug() << "InitialTables - table already exists:" << table_name;
        }
    }

    return true;
}

bool Q1Context::InitialColumns(const QList<Q1Table*>& tables)
{
    if (!query) return false;

    for (Q1Table* table : tables)
    {
        if (!table) continue;

        QString table_name = table->GetName();
        QList<Q1Column> declaredColumns = table->GetColumns();
        QList<Q1Column> existingColumns = query->GetColumns(table_name);
        if (!query->ErrorMessage().isEmpty()) return false;
        if (existingColumns.isEmpty()) {
            model_error = QString("Cannot read columns for table '%1'.").arg(table_name);
            return false;
        }

        // Index the live schema once: matching is linear in the column count.
        // Unmapped columns belong to the database and are preserved.
        QHash<QString, Q1Column> columnsByName;
        for (const Q1Column &column : existingColumns)
            columnsByName.insert(column.name.toLower(), column);

        for (Q1Column &declared : declaredColumns)
        {
            auto existing = columnsByName.find(declared.name.toLower());
            if (existing == columnsByName.end()) {
                if (!query->AddColumn(table_name, declared)) return false;
            } else if (!CompareColumn(table_name, existing.value(), declared)) {
                return false;
            }
        }
    }

    return true;
}

bool Q1Context::CompareColumn(const QString &table_name, Q1Column &dbColumn, Q1Column &declColumn)
{
    if (!query) return false;
    if (dbColumn.type != declColumn.type || dbColumn.primary_key != declColumn.primary_key) {
        model_error = QString("Changing type or primary key for '%1.%2' requires an explicit database schema change.")
                          .arg(table_name, dbColumn.name);
        return false;
    }

    // SQLite does not enforce VARCHAR lengths; integer storage sizes are not
    // character limits and must never trigger ALTER ... TYPE VARCHAR.
    if (!connection->IsSqlite() && (declColumn.type == VARCHAR || declColumn.type == CHAR)
        && declColumn.size > 0 && dbColumn.size != declColumn.size
        && !query->UpdateColumnSize(table_name, declColumn.name, declColumn.size))
        return false;

    if (dbColumn.nullable != declColumn.nullable) {
        if (declColumn.nullable) {
            if (!query->SetColumnNullable(table_name, dbColumn.name)) return false;
        } else {
            const bool hasNull = query->HasNullData(table_name, dbColumn.name);
            if (!query->ErrorMessage().isEmpty()) return false;
            if (hasNull) {
                model_error = QString("Cannot make '%1.%2' required while NULL values exist. Backfill them first.")
                                  .arg(table_name, dbColumn.name);
                return false;
            }
            if (!query->DropColumnNullable(table_name, dbColumn.name)) return false;
        }
    }

    const bool dbIdentity = dbColumn.is_identity || Q1Column::IsIdentityDefault(dbColumn.default_value);
    const bool declIdentity = declColumn.is_identity || Q1Column::IsIdentityDefault(declColumn.default_value);
    if (!connection->IsSqlite() && dbIdentity != declIdentity) {
        model_error = QString("Changing identity generation for '%1.%2' requires an explicit database schema change.")
                          .arg(table_name, dbColumn.name);
        return false;
    }
    if (!dbIdentity && !declIdentity &&
        !Q1Column::DefaultsMatch(dbColumn.default_value, declColumn.default_value)) {
        if (!declColumn.default_value.isEmpty()) {
            if (!query->setColumnDefault(table_name, declColumn.name, declColumn.default_value)) return false;
        } else if (!dbColumn.default_value.isEmpty()) {
            if (!query->DropColumnDefault(table_name, declColumn.name)) return false;
        }
    }
    return true;
}

bool Q1Context::InitialRelations(const QList<Q1Relation> &relations)
{
    if (!query || !connection)
        return false;

    if (!connection->Connect())
        return false;

    const auto disconnect = qScopeGuard([this] { connection->Disconnect(); });
    QStringList existingTables = connection->database.tables();
    for (QString &t : existingTables) t = t.toLower();

    for (const Q1Relation &rel : relations)
    {
        if (rel.base_table.isEmpty() || rel.top_table.isEmpty())
            continue;

        if (!existingTables.contains(rel.base_table.toLower()) ||
            !existingTables.contains(rel.top_table.toLower()))
        {
            model_error = QString("Tables missing for relation %1 -> %2.")
                              .arg(rel.base_table, rel.top_table);
            return false;
        }

        const QString constraint_name = rel.GetConstraintName();

        const bool exists = query->ConstraintExists(connection->database, constraint_name.toLower());
        if (!query->ErrorMessage().isEmpty()) return false;
        if (exists)
        {
            qDebug() << "[Info] Relation already exists, skipping:" << constraint_name;
            continue;
        }

        if (!query->AddRelation(rel)) {
            qWarning() << "[Error] Failed to create relation:" << query->ErrorMessage();
            return false;
        }
        else
            qDebug() << "[Info] Relation created successfully:" << constraint_name;
    }

    return true;
}
