#include "Q1Context.h"
#include <algorithm>

Q1Context::~Q1Context()
{
    if (query)
    {
        delete query;
        query = nullptr;
    }

    if (connection)
    {
        connection->Disconnect();
        connection->RootDisconnect();
        if (owns_connection)
            delete connection;
        connection = nullptr;
    }
}

bool Q1Context::Initialize()
{
    OnConfiguration();

    if (!connection)
    {
        qWarning() << "Q1Context::Initialize - connection is null!";
        return false;
    }

    database_name = connection->GetDatabaseName();

    if (query)
    {
        delete query;
        query = nullptr;
    }

    query = new Q1Migration(*connection);

    InitialDatabase();

    if (!connection->IsOpen())
    {
        qCritical() << "Q1Context::Initialize - connection is not open after InitialDatabase!";
        return false;
    }

    tables = OnTablesCreating();

    InitialTables();
    InitialColumns();

    QList<Q1Relation> allRelations = OnTableRelationCreating();
    InitialRelations(allRelations);

    return true;
}

void Q1Context::InitialDatabase()
{
    if (!connection || !query) return;

    if (!connection->RootConnect())
    {
        qCritical() << "Cannot connect to server:" << connection->ErrorMessage();
        return;
    }

    QStringList databases = query->GetDatabases();
    if (!databases.contains(database_name))
    {
        qDebug() << "InitialDatabase - database not found. Creating:" << database_name;

        if (!query->AddDatabase(database_name))
        {
            qCritical() << "Failed to create database:" << connection->ErrorMessage();
            connection->RootDisconnect();
            return;
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
        return;
    }
}

void Q1Context::InitialTables()
{
    if (!query || !connection) return;

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
}

void Q1Context::InitialColumns()
{
    if (!query) return;

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

            if (!found)
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
}

void Q1Context::CompareColumn(const QString &table_name, Q1Column &dbColumn, Q1Column &declColumn)
{
    if (!query) return;

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

void Q1Context::InitialRelations(const QList<Q1Relation> &relations)
{
    if (!query || !connection)
        return;

    if (!connection->Connect())
        return;

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
}
