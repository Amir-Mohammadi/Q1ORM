#include "Q1Context.h"

Q1Context::~Q1Context()
{
    if(connection != nullptr)
    {
        delete connection;
        connection = nullptr;
    }

    if(query != nullptr)
    {
        delete query;
        query = nullptr;
    }
}

void Q1Context::Initialize()
{
    OnConfiguration();

    if(connection == nullptr)
    {
        return;
    }
    
    query = new Q1Migration(*connection);
    database_name = connection->GetDatabaseName();

    InitialDatabase();

    q1tables = OnTablesCreating();
    InitialTables();
}

void Q1Context::InitialDatabase()
{
    QStringList databases = query->GetDatabases();
    bool has_database = databases.contains(database_name);

    if(has_database)
    {
        return;
    }

    query->AddDatabase(database_name);
}

void Q1Context::InitialTables()
{
    connection->Connect();
    tables = connection->database.tables();
    connection->Disconnect();

    for(Q1Table q1table : q1tables)
    {
        if(tables.contains(q1table.table_name))
        {
            if(check_columns)
            {
                InitialColumns(q1table);
            }

            continue;
        }

        query->AddTable(q1table);
        tables.append(q1table.table_name);
    }

    bool drop_table = true;

    for(QString table : tables)
    {
        for(Q1Table q1table : q1tables)
        {
            if(q1table.table_name == table)
            {
                drop_table = false;
                break;
            }
        }

        if(drop_table)
        {
            query->DropTable(table);
        }

        drop_table = true;
    }
}

void Q1Context::InitialColumns(Q1Table &q1table)
{
    QList<Q1Column> columns = query->GetColumns(q1table.table_name);
    bool drop_column = true;

    for(Q1Column column : columns)
    {
        for(Q1Column q1column : q1table.columns)
        {
            if(q1column == column)
            {
                CompareColumn(q1table.table_name, column, q1column);
                drop_column = false;

                break;
            }
        }

        if(drop_column)
        {
            query->DropColumn(q1table.table_name, column.name);
        }

        drop_column = true;
    }

    for(Q1Column q1column : q1table.columns)
    {
        int index_of = Q1Column::IndexOf(columns, q1column);

        if(index_of != -1)
        {
            continue;
        }

        query->AddColumn(q1table.table_name, q1column);
    }
}

void Q1Context::CompareColumn(QString table_name, Q1Column &column, Q1Column &q1column)
{
    if(column.size != q1column.size)
    {
        query->UpdateColumnSize(table_name, q1column.name, q1column.size);
    }

    if(column.nullable != q1column.nullable)
    {
        if(column.nullable  && !q1column.nullable)
        {
            if(!query->HasNullData(table_name, q1column.name))
            {
                query->DropColumnNullable(table_name, q1column.name);
            }
        }
        else
        {
            query->SetColumnNullable(table_name, q1column.name);
        }
    }

    if(column.default_value != q1column.default_value)
    {
        if(q1column.default_value != "")
        {
            query->setColumnDefault(table_name, q1column.name, q1column.default_value);
        }
        else if(column.default_value != "")
        {
            query->DropColumnDefault(table_name, q1column.name);
        }
    }
}
