#include "Q1MigrationQuery.h"


/* ############################################################################### */
/* ************************************ Get ************************************** */
/* ############################################################################### */

QString Q1MigrationQuery::GetDatabases()
{
    QString query = "SELECT datname FROM pg_database";

    return query;
}

QString Q1MigrationQuery::GetColumns(QString table_name)
{
    QString query = "SELECT column_name, data_type, character_maximum_length, is_nullable, column_default, ordinal_position,"
                    "(SELECT constraint_name FROM information_schema.key_column_usage WHERE table_name='" + table_name + "' AND column_name = c.column_name) "
                                   "AS constraint_name FROM information_schema.columns c WHERE table_name='" + table_name + "'";

    return query;
}


/* ############################################################################### */
/* ************************************ Add ************************************** */
/* ############################################################################### */

QString Q1MigrationQuery::AddDatabase(QString database_name, QString username)
{
    QString query = "CREATE DATABASE \"" + database_name + "\" WITH OWNER=" + username + " ENCODING='UTF8' CONNECTION LIMIT=-1 IS_TEMPLATE=False";

    return query;
}

QString Q1MigrationQuery::AddTable(Q1Table &q1table)
{
    QString comma = ", ";
    QString query = "CREATE TABLE \"";

    query.append(q1table.table_name);
    query.append("\"(");

    for (int i = 0; i < q1table.columns.length(); i++)
    {
        QString column_property = ColumnProperty(q1table.columns[i]);
        query.append(column_property);

        if (i != q1table.columns.length() - 1)
        {
            query.append(comma);
        }
    }

    query.append(")");

    return query;
}

QString Q1MigrationQuery::AddColumn(QString table_name, Q1Column &column)
{
    QString query = "ALTER TABLE IF EXISTS \"" + table_name + "\" ADD COLUMN " + ColumnProperty(column);

    return query;
}


/* ############################################################################### */
/* *********************************** Drop ************************************** */
/* ############################################################################### */

QString Q1MigrationQuery::DropTable(QString table_name)
{
    QString query = "DROP TABLE \"" + table_name + "\"";

    return query;
}

QString Q1MigrationQuery::DropColumn(QString table_name, QString column_name)
{
    QString query = "ALTER TABLE \"" + table_name + "\" DROP COLUMN \"" + column_name + "\"";

    return query;
}

QString Q1MigrationQuery::DropColumnNullable(QString table_name, QString column_name)
{
    QString query = "ALTER TABLE \"" + table_name + "\" ALTER COLUMN \"" + column_name + "\" SET NOT NULL";

    return query;
}

QString Q1MigrationQuery::DropColumnDefault(QString table_name, QString column_name)
{
    QString query = "ALTER TABLE \"" + table_name + "\" ALTER COLUMN \"" + column_name + "\" DROP DEFAULT";

    return query;
}


/* ############################################################################### */
/* ************************************ Set ************************************** */
/* ############################################################################### */

QString Q1MigrationQuery::SetColumnNullable(QString table_name, QString column_name)
{
    QString query = "ALTER TABLE \"" + table_name + "\" ALTER COLUMN \"" + column_name + "\" DROP NOT NULL";

    return query;
}

QString Q1MigrationQuery::setColumnDefault(QString table_name, QString column_name, QString default_value)
{
    QString query = "ALTER TABLE \"" + table_name + "\" ALTER COLUMN \"" + column_name + "\" SET DEFAULT " + default_value;

    return query;
}


/* ############################################################################### */
/* ********************************* Update ************************************** */
/* ############################################################################### */

QString Q1MigrationQuery::UpdateColumnSize(QString table_name, QString column_name, int size)
{
    QString query = "ALTER TABLE \"" + table_name + "\" ALTER COLUMN \"" + column_name + "\" TYPE VARCHAR(" + size + ")" ;

    return query;
}


/* ############################################################################### */
/* ************************************ Has ************************************** */
/* ############################################################################### */

QString Q1MigrationQuery::HasNulllData(QString table_name, QString column_name)
{
    QString query = "SELECT " + column_name + " FROM \"" + table_name + "\" WHERE \"" + column_name + "\" IS NULL";

    return query;
}


/* ############################################################################### */
/* ********************************** Private ************************************ */
/* ############################################################################### */

QString Q1MigrationQuery::ColumnProperty(Q1Column &column)
{
    QString query = "";
    QString type,
        size,
        nullable,
        primary_key,
        default_value = nullptr;

    switch (column.type)
    {
        case 0:
        {
            type = "INTEGER";
            size = nullptr;
            break;
        }

        case 1:
        {
            type = "SMALLINT";
            size = nullptr;
            break;
        }

        case 2:
        {
            type = "BIGINT";
            size = nullptr;
            break;
        }

        case 3:
        {
            type = "REAL";
            size = nullptr;
            break;
        }

        case 4:
        {
            type = "DOUBLE PRECISION";
            size = nullptr;
            break;
        }

        case 5:
        {
            type = "BOOLEAN";
            size = nullptr;
            break;
        }

        case 6:
        {
            type = "CHAR";
            size = "(" + QString::number(column.size) + ")";
            break;
        }

        case 7:
        {
            type = "TEXT";
            size = nullptr;
            break;
        }

        case 8:
        {
            type = "VARCHAR";
            size = "(" + QString::number(column.size) + ")";
            break;
        }

        case 9:
        {
            type = "DATE";
            size = nullptr;
            break;
        }

        case 10:
        {
            type = "TIMESTAMP";
            size = nullptr;
            break;
        }
    }

    if(column.nullable)
    {
        if (column.primary_key)
        {
            nullable = "NOT NULL";
        }
        else
        {
            nullable = "NULL";
        }
    }
    else
    {
        nullable = "NOT NULL";
    }

    if(column.primary_key)
    {
        primary_key = "PRIMARY KEY";
    }
    else
    {
        primary_key = "";
    }

    query.append("\"")
        .append(column.name)
        .append("\" ")
        .append(type)
        .append(size)
        .append(" ")
        .append(nullable)
        .append(" ")
        .append(primary_key)
        .append(" ");

    if (column.default_value != nullptr)
    {
        default_value = "DEFAULT ";
        query.append(default_value + "'" + column.default_value + "'");
    }

    return query;
}
