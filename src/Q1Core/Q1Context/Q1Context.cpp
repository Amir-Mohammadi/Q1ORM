#include "Q1Context.h"
#include "../Q1Entity/Q1ModelBuilder.h"

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

    if (!InitialDatabase()) {
        qCritical() << "Q1Context::Initialize - database initialization failed:"
                    << GetLastError();
        return false;
    }

    if (!connection->IsOpen()) {
        qCritical() << "Q1Context::Initialize - connection not open after InitialDatabase!";
        return false;
    }

    if (!query->EnsureHistoryTable()) {
        qCritical() << "Q1Context::Initialize - failed to create migration history table:"
                    << query->ErrorMessage();
        return false;
    }

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

    for (Q1Table *table : tables) {
        if (!table)
            continue;
        if (!query->EnsureIndexes(*table)) {
            qCritical() << "Q1Context::Initialize - index creation failed for"
                        << table->GetName() << ":" << query->ErrorMessage();
            return false;
        }
    }

    if (!InitialRelations(relations)) {
        qCritical() << "Q1Context::Initialize - relation initialization failed:"
                    << GetLastError();
        return false;
    }

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

        // Drop columns not declared
        for (Q1Column &dbCol : existingColumns)
        {
            bool found = false;
            for (Q1Column &declCol : declaredColumns)
            {
                if (declCol == dbCol)
                {
                    CompareColumn(table_name, dbCol, declCol);
                    found = true;
                    break;
                }
            }

            if (!found && allow_destructive_migrations)
            {
                qDebug() << "InitialColumns - dropping column" << dbCol.name << "from" << table_name;
                query->DropColumn(table_name, dbCol.name);
            }
        }

        // Add missing columns
        for (Q1Column &declCol : declaredColumns)
        {
            int idx = Q1Column::IndexOf(existingColumns, declCol);
            if (idx == -1)
            {
                qDebug() << "InitialColumns - adding column" << declCol.name << "to" << table_name;
                query->AddColumn(table_name, declCol);
            }
        }
    }

    return true;
}

void Q1Context::CompareColumn(const QString &table_name, Q1Column &dbColumn, Q1Column &declColumn)
{
    if (!query) return;

    if (dbColumn.type != declColumn.type)
    {
        qWarning() << "Schema type change detected for" << table_name << dbColumn.name
                   << "but automatic type conversion is not implemented";
        return;
    }

    if (dbColumn.size != declColumn.size)
        query->UpdateColumnSize(table_name, declColumn.name, declColumn.size);

    if (dbColumn.nullable != declColumn.nullable)
    {
        if (declColumn.nullable)
        {
            query->SetColumnNullable(table_name, dbColumn.name);
        }
        else
        {
            if (!query->HasNullData(table_name, dbColumn.name))
                query->DropColumnNullable(table_name, dbColumn.name);
        }
    }

    const bool dbIdentity = dbColumn.is_identity || Q1Column::IsIdentityDefault(dbColumn.default_value);
    const bool declIdentity = declColumn.is_identity || Q1Column::IsIdentityDefault(declColumn.default_value);

    if (!dbIdentity && !declIdentity &&
        !Q1Column::DefaultsMatch(dbColumn.default_value, declColumn.default_value))
    {
        if (!declColumn.default_value.isEmpty())
            query->setColumnDefault(table_name, declColumn.name, declColumn.default_value);
        else if (!dbColumn.default_value.isEmpty())
            query->DropColumnDefault(table_name, declColumn.name);
    }
}

bool Q1Context::InitialRelations(const QList<Q1Relation> &relations)
{
    if (!query || !connection)
        return false;

    if (!connection->Connect())
        return false;

    QStringList existingTables = connection->database.tables();
    for (QString &t : existingTables) t = t.toLower();

    for (const Q1Relation &rel : relations)
    {
        if (rel.base_table.isEmpty() || rel.top_table.isEmpty())
            continue;

        if (!existingTables.contains(rel.base_table.toLower()) ||
            !existingTables.contains(rel.top_table.toLower()))
        {
            qDebug() << "[Debug] Tables missing, skipping relation:"
                     << rel.base_table << "->" << rel.top_table;
            continue;
        }

        const QString constraint_name = rel.GetConstraintName();

        if (query->ConstraintExists(connection->database, constraint_name.toLower()))
        {
            qDebug() << "[Info] Relation already exists, skipping:" << constraint_name;
            continue;
        }

        if (!query->AddRelation(rel))
            qWarning() << "[Error] Failed to create relation:" << query->ErrorMessage();
        else
            qDebug() << "[Info] Relation created successfully:" << constraint_name;
    }

    connection->Disconnect();
    return true;
}
