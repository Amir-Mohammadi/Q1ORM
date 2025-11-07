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
    QString query = "CREATE TABLE IF NOT EXISTS \"";

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
    QString query;
    QString type;
    QString size;

    // Map C++ type to SQL type
    switch (column.type)
    {
    case 0: type = "INTEGER"; break;
    case 1: type = "SMALLINT"; break;
    case 2: type = "BIGINT"; break;
    case 3: type = "REAL"; break;
    case 4: type = "DOUBLE PRECISION"; break;
    case 5: type = "BOOLEAN"; break;
    case 6: type = "CHAR"; size = "(" + QString::number(column.size) + ")"; break;
    case 7: type = "TEXT"; break;
    case 8: type = "VARCHAR"; size = "(" + QString::number(column.size) + ")"; break;
    case 9: type = "DATE"; break;
    case 10: type = "TIMESTAMP"; break;
    default: type = "VARCHAR"; break;
    }

    // Handle identity/auto-increment primary key
    if (column.primary_key && column.default_value == "GENERATED ALWAYS AS IDENTITY")
    {
        query = "\"" + column.name + "\" INTEGER GENERATED ALWAYS AS IDENTITY PRIMARY KEY";
        return query;
    }

    // Nullable or NOT NULL
    QString nullable = column.nullable ? "" : "NOT NULL";

    // Primary key (if not identity)
    QString primary_key = (column.primary_key && column.default_value != "GENERATED ALWAYS AS IDENTITY") ? "PRIMARY KEY" : "";

    // Default value
    QString default_clause;
    if (!column.default_value.isEmpty())
    {
        // Do NOT wrap integer/boolean defaults in quotes
        if (column.type == INTEGER || column.type == SMALLINT || column.type == BIGINT || column.type == REAL || column.type == DOUBLE_PRECISION || column.type == BOOLEAN)
            default_clause = "DEFAULT " + column.default_value;
        else
            default_clause = "DEFAULT '" + column.default_value + "'";
    }

    // Build full column definition
    query = "\"" + column.name + "\" " + type + size + " " + nullable + " " + primary_key + " " + default_clause;

    return query.trimmed();
}



QString Q1MigrationQuery::AddRelationSQL(const Q1Relation &relation)
{
    if (!relation.IsValid()) {
        m_lastError = "Invalid relation";
        return "";
    }

    QString fkName = relation.GetConstraintName();
    QString fkBase;    // table that will hold the FK column
    QString fkTop;     // referenced table
    QString fkColumn;  // FK column in fkBase
    QString fkRefCol;  // referenced column in fkTop

    // Determine FK/Parent orientation based on relation type
    switch (relation.type)
    {

        case ONE_TO_ONE:
            fkBase   = relation.base_table;
            fkTop    = relation.top_table;
            fkColumn = relation.foreign_key;
            fkRefCol = relation.reference_key;
            break;

        case ONE_TO_MANY:
            // base = parent, top = child
            fkBase   = relation.top_table;     // FK lives in child
            fkTop    = relation.base_table;    // parent table is referenced
            fkColumn = relation.reference_key; // child column
            fkRefCol = relation.foreign_key;   // parent column
            break;

        case MANY_TO_ONE:
            // base = child, top = parent
            fkBase   = relation.base_table;    // child table holds FK
            fkTop    = relation.top_table;     // parent table is referenced
            fkColumn = relation.foreign_key;   // child column
            fkRefCol = relation.reference_key; // parent column
            break;

        case MANY_TO_MANY:
        {
            // create junction table
            QString junction = relation.base_table + "_" + relation.top_table;
            QString base_col = relation.base_table + "_" + relation.foreign_key;
            QString top_col  = relation.top_table + "_" + relation.reference_key;

            return QString(
                       "CREATE TABLE IF NOT EXISTS \"%1\" ("
                       "\"%2\" INTEGER NOT NULL, "
                       "\"%3\" INTEGER NOT NULL, "
                       "PRIMARY KEY (\"%2\", \"%3\"), "
                       "CONSTRAINT fk_%1_%2 FOREIGN KEY (\"%2\") REFERENCES \"%4\"(\"%5\") ON DELETE CASCADE, "
                       "CONSTRAINT fk_%1_%3 FOREIGN KEY (\"%3\") REFERENCES \"%6\"(\"%7\") ON DELETE CASCADE)"
                       ).arg(junction, base_col, top_col, relation.base_table, relation.foreign_key, relation.top_table, relation.reference_key);
        }
    }

    if (relation.type == ONE_TO_ONE) {
        // UNIQUE + FK
        QString uqName = QString("UQ_%1_%2").arg(fkBase, fkColumn);
        QString uqSql  = QString("ALTER TABLE \"%1\" ADD CONSTRAINT \"%2\" UNIQUE (\"%3\")")
                            .arg(fkBase, uqName, fkColumn);
        QString fkSql  = QString("ALTER TABLE \"%1\" ADD CONSTRAINT \"%2\" FOREIGN KEY (\"%3\") REFERENCES \"%4\" (\"%5\") ON DELETE %6 ON UPDATE %7")
                            .arg(fkBase, fkName, fkColumn, fkTop, fkRefCol, relation.GetOnDeleteString(), relation.GetOnUpdateString());
        return uqSql + ";" + fkSql;
    }
    else {
        // typical FK for ONE_TO_MANY, MANY_TO_ONE
        return QString("ALTER TABLE \"%1\" ADD CONSTRAINT \"%2\" FOREIGN KEY (\"%3\") REFERENCES \"%4\" (\"%5\") ON DELETE %6 ON UPDATE %7")
            .arg(fkBase, fkName, fkColumn, fkTop, fkRefCol, relation.GetOnDeleteString(), relation.GetOnUpdateString());
    }
}




