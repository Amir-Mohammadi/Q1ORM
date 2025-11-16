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
        delete connection;
        connection = nullptr;
    }
}


void Q1Context::Initialize()
{
    OnConfiguration();

    if (!connection)
    {
        qWarning() << "Q1Context::Initialize - connection is null!";
        return;
    }

    if (query)
    {
        delete query;
        query = nullptr;
    }

    query = new Q1Migration(*connection);
    database_name = connection->GetDatabaseName();

    // 1) Ensure database exists
    InitialDatabase();

    // 2) Get table definitions
    q1tables = OnTablesCreating();

    // 3) Create tables first
    InitialTables(); // Create tables and columns, no relations yet

    // 4) Ensure all parent tables exist before creating relations
    QList<Q1Relation> allRelations = OnTableRelationCreating();

    // 5) Create relations safely
    InitialRelations(allRelations);
}

void Q1Context::InitialDatabase()
{
    if (!connection || !query) return;

    // 1) Connect to server using root connection
    if (!connection->RootConnect())
    {
        qCritical() << "Cannot connect to server:" << connection->ErrorMessage();
        return;
    }

    // 2) Check if the database exists
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

    // 3) Disconnect from server-level connection
    connection->RootDisconnect();

    // 4) Connect normally to the newly created database
    if (!connection->Connect())
    {
        qCritical() << "Failed to connect to database" << database_name
                    << "-" << connection->ErrorMessage();
        return;
    }
}


void Q1Context::InitialTables()
{
    if (!connection || !query)
        return;

    if (!connection->Connect())
    {
        qWarning() << "InitialTables - failed to connect to database:" << connection->ErrorMessage();
        return;
    }

    // Get existing tables once
    QStringList existingTables = connection->database.tables();

    for (Q1Table &declaredTable : q1tables)
    {
        if (!existingTables.contains(declaredTable.table_name))
        {
            qDebug() << "InitialTables - creating table:" << declaredTable.table_name;
            query->AddTable(declaredTable);
            existingTables.append(declaredTable.table_name);
        }

        if (check_columns)
            InitialColumns(declaredTable);
    }

    connection->Disconnect();
}


void Q1Context::InitialColumns(Q1Table &q1table)
{
    if (!query) return;

    QList<Q1Column> existingColumns = query->GetColumns(q1table.table_name);

    // Drop columns not declared
    for (Q1Column &dbCol : existingColumns)
    {
        bool found = false;
        for (Q1Column &declCol : q1table.columns)
        {
            if (declCol == dbCol)
            {
                CompareColumn(q1table.table_name, dbCol, declCol);
                found = true;
                break;
            }
        }

        if (!found)
        {
            qDebug() << "InitialColumns - dropping column" << dbCol.name << "from" << q1table.table_name;
            query->DropColumn(q1table.table_name, dbCol.name);
        }
    }

    // Add missing columns
    for (Q1Column &declCol : q1table.columns)
    {
        int idx = Q1Column::IndexOf(existingColumns, declCol);
        if (idx == -1)
        {
            qDebug() << "InitialColumns - adding column" << declCol.name << "to" << q1table.table_name;
            query->AddColumn(q1table.table_name, declCol);
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
        if (declColumn.nullable && !dbColumn.nullable)
        {
            if (!query->HasNullData(table_name, dbColumn.name))
                query->DropColumnNullable(table_name, dbColumn.name);
        }
        else
        {
            query->SetColumnNullable(table_name, dbColumn.name);
        }
    }

    if (dbColumn.default_value != declColumn.default_value)
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
    for (QString &t : existingTables) t = t.toLower(); // normalize

    for (const Q1Relation &rel : relations)
    {
        if (rel.base_table.isEmpty() || rel.top_table.isEmpty())
            continue;

        // check tables (case-insensitive)
        if (!existingTables.contains(rel.base_table.toLower()) ||
            !existingTables.contains(rel.top_table.toLower()))
        {
            qDebug() << "[Debug] Tables missing, skipping relation:"
                     << rel.base_table << "->" << rel.top_table;
            continue;
        }

        const QString constraint_name = rel.GetConstraintName();

        // Skip if constraint already exists
        if (query->ConstraintExists(connection->database, constraint_name.toLower()))
        {
            qDebug() << "[Info] Relation already exists, skipping:" << constraint_name;
            continue;
        }

        // Add relation
        if (!query->AddRelation(rel))
            qWarning() << "[Error] Failed to create relation:" << query->ErrorMessage();
        else
            qDebug() << "[Info] Relation created successfully:" << constraint_name;
    }

    connection->Disconnect();
}
