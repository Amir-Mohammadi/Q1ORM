#include "Q1MigrationQuery.h"

#include <QStringList>

Q1MigrationQuery::Q1MigrationQuery(DatabaseType type)
    : db_type(type)
{
}

QString Q1MigrationQuery::GetDatabasesSQL()
{
    switch (db_type)
    {
    case DatabaseType::SQLServer:
        return "SELECT name FROM sys.databases ORDER BY name";
    case DatabaseType::PostgreSQL:
    case DatabaseType::MySQL:
    case DatabaseType::SQLite:
    default:
        return "SELECT datname FROM pg_database WHERE datistemplate = false";
    }
}

QString Q1MigrationQuery::GetColumnsSQL(QString table_name)
{
    switch (db_type)
    {
    case DatabaseType::SQLServer:
        return QString(
                   "SELECT c.COLUMN_NAME AS column_name, "
                   "c.DATA_TYPE AS data_type, "
                   "c.CHARACTER_MAXIMUM_LENGTH AS character_maximum_length, "
                   "c.IS_NULLABLE AS is_nullable, "
                   "c.COLUMN_DEFAULT AS column_default, "
                   "CAST(COLUMNPROPERTY(OBJECT_ID(QUOTENAME(c.TABLE_SCHEMA) + '.' + QUOTENAME(c.TABLE_NAME)), c.COLUMN_NAME, 'IsIdentity') AS INT) AS is_identity, "
                   "c.ORDINAL_POSITION AS ordinal_position, "
                   "(SELECT TOP 1 tc.CONSTRAINT_NAME "
                   " FROM INFORMATION_SCHEMA.KEY_COLUMN_USAGE kcu "
                   " JOIN INFORMATION_SCHEMA.TABLE_CONSTRAINTS tc "
                   "   ON tc.CONSTRAINT_NAME = kcu.CONSTRAINT_NAME "
                   "  AND tc.TABLE_SCHEMA = kcu.TABLE_SCHEMA "
                   " WHERE kcu.TABLE_NAME = c.TABLE_NAME "
                   "   AND kcu.COLUMN_NAME = c.COLUMN_NAME "
                   "   AND tc.CONSTRAINT_TYPE = 'PRIMARY KEY') AS constraint_name "
                   "FROM INFORMATION_SCHEMA.COLUMNS c "
                   "WHERE c.TABLE_NAME = '%1' "
                   "ORDER BY c.ORDINAL_POSITION")
            .arg(EscapeSqlString(table_name));
    case DatabaseType::PostgreSQL:
    case DatabaseType::MySQL:
    case DatabaseType::SQLite:
    default:
        return QString(
                   "SELECT column_name, data_type, character_maximum_length, is_nullable, column_default, is_identity, ordinal_position, "
                   "(SELECT constraint_name FROM information_schema.key_column_usage "
                   "WHERE table_name='%1' AND column_name = c.column_name) AS constraint_name "
                   "FROM information_schema.columns c WHERE table_name='%1' ORDER BY ordinal_position")
            .arg(EscapeSqlString(table_name));
    }
}

QString Q1MigrationQuery::ConstraintExistsSQL(const QString &constraint_name)
{
    switch (db_type)
    {
    case DatabaseType::SQLServer:
        return QString(
                   "SELECT COUNT(*) FROM INFORMATION_SCHEMA.TABLE_CONSTRAINTS "
                   "WHERE LOWER(CONSTRAINT_NAME) = LOWER('%1')")
            .arg(EscapeSqlString(constraint_name));
    case DatabaseType::PostgreSQL:
    case DatabaseType::MySQL:
    case DatabaseType::SQLite:
    default:
        return QString(
                   "SELECT COUNT(*) FROM information_schema.table_constraints "
                   "WHERE LOWER(constraint_name) = LOWER('%1')")
            .arg(EscapeSqlString(constraint_name));
    }
}

QString Q1MigrationQuery::AddDatabaseSQL(QString database_name)
{
    switch (db_type)
    {
    case DatabaseType::SQLServer:
        return QString("IF DB_ID(N'%1') IS NULL CREATE DATABASE %2")
            .arg(EscapeSqlString(database_name), QuoteIdentifier(database_name));
    case DatabaseType::PostgreSQL:
    case DatabaseType::MySQL:
    case DatabaseType::SQLite:
    default:
        return QString("CREATE DATABASE \"%1\" WITH ENCODING='UTF8' CONNECTION LIMIT=-1").arg(database_name);
    }
}

QString Q1MigrationQuery::AddTableSQL(Q1Table &q1table)
{
    if (q1table.columns.isEmpty())
    {
        qWarning() << "AddTableSQL: table has no columns:" << q1table.table_name;
        return "";
    }

    QStringList column_defs;
    for (const Q1Column &column : q1table.columns)
    {
        const QString column_definition = ColumnProperty(column);
        if (!column_definition.isEmpty())
            column_defs.append(column_definition);
    }

    switch (db_type)
    {
    case DatabaseType::SQLServer:
        return QString("IF OBJECT_ID(N'%1', N'U') IS NULL CREATE TABLE %2 (%3)")
            .arg(EscapeSqlString(q1table.table_name),
                 QuoteIdentifier(q1table.table_name),
                 column_defs.join(", "));
    case DatabaseType::PostgreSQL:
    case DatabaseType::MySQL:
    case DatabaseType::SQLite:
    default:
        return QString("CREATE TABLE IF NOT EXISTS \"%1\" (%2)")
            .arg(q1table.table_name, column_defs.join(", "));
    }
}

QString Q1MigrationQuery::AddColumnSQL(QString table_name, const Q1Column &column)
{
    switch (db_type)
    {
    case DatabaseType::SQLServer:
        return QString("ALTER TABLE %1 ADD %2")
            .arg(QuoteIdentifier(table_name), ColumnProperty(column));
    case DatabaseType::PostgreSQL:
    case DatabaseType::MySQL:
    case DatabaseType::SQLite:
    default:
        return QString("ALTER TABLE \"%1\" ADD COLUMN %2").arg(table_name, ColumnProperty(column));
    }
}

QString Q1MigrationQuery::AddRelationSQL(const Q1Relation &relation)
{
    if (!relation.IsValid())
    {
        m_lastError = "Invalid relation";
        return "";
    }

    const QString fkName = relation.GetConstraintName();
    QString fkBase;
    QString fkTop;
    QString fkColumn;
    QString fkRefCol;

    switch (relation.type)
    {
    case ONE_TO_ONE:
        fkBase = relation.base_table;
        fkTop = relation.top_table;
        fkColumn = relation.foreign_key;
        fkRefCol = relation.reference_key;
        break;
    case ONE_TO_MANY:
        fkBase = relation.top_table;
        fkTop = relation.base_table;
        fkColumn = relation.foreign_key;
        fkRefCol = relation.reference_key;
        break;
    case MANY_TO_ONE:
        fkBase = relation.base_table;
        fkTop = relation.top_table;
        fkColumn = relation.foreign_key;
        fkRefCol = relation.reference_key;
        break;
    case MANY_TO_MANY:
    {
        const QString junction = relation.base_table + "_" + relation.top_table;
        const QString base_col = relation.base_table + "_" + relation.foreign_key;
        const QString top_col = relation.top_table + "_" + relation.reference_key;

        if (db_type == DatabaseType::SQLServer)
        {
            return QString(
                       "IF OBJECT_ID(N'%1', N'U') IS NULL "
                       "CREATE TABLE %2 ("
                       "%3 INT NOT NULL, "
                       "%4 INT NOT NULL, "
                       "CONSTRAINT %5 PRIMARY KEY (%3, %4), "
                       "CONSTRAINT %6 FOREIGN KEY (%3) REFERENCES %7(%8) ON DELETE CASCADE, "
                       "CONSTRAINT %9 FOREIGN KEY (%4) REFERENCES %10(%11) ON DELETE CASCADE)")
                .arg(EscapeSqlString(junction),
                     QuoteIdentifier(junction),
                     QuoteIdentifier(base_col),
                     QuoteIdentifier(top_col),
                     QuoteIdentifier(QString("PK_%1").arg(junction)),
                     QuoteIdentifier(QString("fk_%1_%2").arg(junction, base_col)),
                     QuoteIdentifier(relation.base_table),
                     QuoteIdentifier(relation.foreign_key),
                     QuoteIdentifier(QString("fk_%1_%2").arg(junction, top_col)),
                     QuoteIdentifier(relation.top_table),
                     QuoteIdentifier(relation.reference_key));
        }

        return QString(
                   "CREATE TABLE IF NOT EXISTS \"%1\" ("
                   "\"%2\" INTEGER NOT NULL, "
                   "\"%3\" INTEGER NOT NULL, "
                   "PRIMARY KEY (\"%2\", \"%3\"), "
                   "CONSTRAINT fk_%1_%2 FOREIGN KEY (\"%2\") REFERENCES \"%4\"(\"%5\") ON DELETE CASCADE, "
                   "CONSTRAINT fk_%1_%3 FOREIGN KEY (\"%3\") REFERENCES \"%6\"(\"%7\") ON DELETE CASCADE)")
            .arg(junction, base_col, top_col, relation.base_table, relation.foreign_key,
                 relation.top_table, relation.reference_key);
    }
    }

    if (db_type == DatabaseType::SQLServer)
    {
        const QString baseTable = QuoteIdentifier(fkBase);
        const QString topTable = QuoteIdentifier(fkTop);
        const QString foreignKey = QuoteIdentifier(fkColumn);
        const QString referenceKey = QuoteIdentifier(fkRefCol);
        const QString quotedConstraint = QuoteIdentifier(fkName);

        if (relation.type == ONE_TO_ONE)
        {
            const QString uqName = QuoteIdentifier(QString("uq_%1_%2").arg(fkBase, fkColumn).toLower());
            const QString uqSql = QString("ALTER TABLE %1 ADD CONSTRAINT %2 UNIQUE (%3)")
                                      .arg(baseTable, uqName, foreignKey);
            const QString fkSql = QString("ALTER TABLE %1 ADD CONSTRAINT %2 FOREIGN KEY (%3) "
                                          "REFERENCES %4(%5) ON DELETE %6 ON UPDATE %7")
                                      .arg(baseTable, quotedConstraint, foreignKey, topTable, referenceKey,
                                           relation.GetOnDeleteString(), relation.GetOnUpdateString());
            return uqSql + "; " + fkSql;
        }

        return QString("ALTER TABLE %1 ADD CONSTRAINT %2 FOREIGN KEY (%3) "
                       "REFERENCES %4(%5) ON DELETE %6 ON UPDATE %7")
            .arg(baseTable, quotedConstraint, foreignKey, topTable, referenceKey,
                 relation.GetOnDeleteString(), relation.GetOnUpdateString());
    }

    if (relation.type == ONE_TO_ONE)
    {
        const QString uqName = QString("uq_%1_%2").arg(fkBase, fkColumn).toLower();
        const QString uqSql = QString("ALTER TABLE \"%1\" ADD CONSTRAINT \"%2\" UNIQUE (\"%3\")")
                                  .arg(fkBase, uqName, fkColumn);
        const QString fkSql = QString("ALTER TABLE \"%1\" ADD CONSTRAINT \"%2\" FOREIGN KEY (\"%3\") "
                                      "REFERENCES \"%4\"(\"%5\") ON DELETE %6 ON UPDATE %7")
                                  .arg(fkBase, fkName, fkColumn, fkTop, fkRefCol,
                                       relation.GetOnDeleteString(), relation.GetOnUpdateString());
        return uqSql + "; " + fkSql;
    }

    return QString("ALTER TABLE \"%1\" ADD CONSTRAINT \"%2\" FOREIGN KEY (\"%3\") "
                   "REFERENCES \"%4\"(\"%5\") ON DELETE %6 ON UPDATE %7")
        .arg(fkBase, fkName, fkColumn, fkTop, fkRefCol,
             relation.GetOnDeleteString(), relation.GetOnUpdateString());
}

QString Q1MigrationQuery::DropTableSQL(QString table_name)
{
    switch (db_type)
    {
    case DatabaseType::SQLServer:
        return QString("IF OBJECT_ID(N'%1', N'U') IS NOT NULL DROP TABLE %2")
            .arg(EscapeSqlString(table_name), QuoteIdentifier(table_name));
    case DatabaseType::PostgreSQL:
    case DatabaseType::MySQL:
    case DatabaseType::SQLite:
    default:
        return QString("DROP TABLE IF EXISTS \"%1\" CASCADE").arg(table_name);
    }
}

QString Q1MigrationQuery::DropColumnSQL(QString table_name, QString column_name)
{
    switch (db_type)
    {
    case DatabaseType::SQLServer:
        return QString(
                   "IF EXISTS (SELECT 1 FROM sys.columns "
                   "WHERE object_id = OBJECT_ID(N'%1') AND name = N'%2') "
                   "ALTER TABLE %3 DROP COLUMN %4")
            .arg(EscapeSqlString(table_name),
                 EscapeSqlString(column_name),
                 QuoteIdentifier(table_name),
                 QuoteIdentifier(column_name));
    case DatabaseType::PostgreSQL:
    case DatabaseType::MySQL:
    case DatabaseType::SQLite:
    default:
        return QString("ALTER TABLE \"%1\" DROP COLUMN IF EXISTS \"%2\" CASCADE").arg(table_name, column_name);
    }
}

QString Q1MigrationQuery::DropColumnNullableSQL(QString table_name, QString column_name)
{
    if (db_type == DatabaseType::SQLServer)
        return AlterColumnNullabilitySQL(table_name, column_name, false);

    return QString("ALTER TABLE \"%1\" ALTER COLUMN \"%2\" SET NOT NULL").arg(table_name, column_name);
}

QString Q1MigrationQuery::DropColumnDefaultSQL(QString table_name, QString column_name)
{
    if (db_type == DatabaseType::SQLServer)
        return DropDefaultConstraintSQL(table_name, column_name);

    return QString("ALTER TABLE \"%1\" ALTER COLUMN \"%2\" DROP DEFAULT").arg(table_name, column_name);
}

QString Q1MigrationQuery::SetColumnNullableSQL(QString table_name, QString column_name)
{
    if (db_type == DatabaseType::SQLServer)
        return AlterColumnNullabilitySQL(table_name, column_name, true);

    return QString("ALTER TABLE \"%1\" ALTER COLUMN \"%2\" DROP NOT NULL").arg(table_name, column_name);
}

QString Q1MigrationQuery::SetColumnDefaultSQL(QString table_name, QString column_name, QString default_value)
{
    if (db_type == DatabaseType::SQLServer)
    {
        return QString("%1; ALTER TABLE %2 ADD CONSTRAINT %3 DEFAULT %4 FOR %5")
            .arg(DropDefaultConstraintSQL(table_name, column_name),
                 QuoteIdentifier(table_name),
                 QuoteIdentifier(QString("DF_%1_%2").arg(table_name, column_name)),
                 FormatDefaultExpression(default_value),
                 QuoteIdentifier(column_name));
    }

    return QString("ALTER TABLE \"%1\" ALTER COLUMN \"%2\" SET DEFAULT %3").arg(table_name, column_name, default_value);
}

QString Q1MigrationQuery::UpdateColumnSizeSQL(QString table_name, QString column_name, int size)
{
    if (db_type == DatabaseType::SQLServer)
    {
        return QString(
                   "DECLARE @is_nullable NVARCHAR(8) = N'NULL'; "
                   "IF EXISTS (SELECT 1 FROM sys.columns WHERE object_id = OBJECT_ID(N'%1') AND name = N'%2' AND is_nullable = 0) "
                   "SET @is_nullable = N'NOT NULL'; "
                   "EXEC(N'ALTER TABLE %3 ALTER COLUMN %4 NVARCHAR(%5) ' + @is_nullable);")
            .arg(EscapeSqlString(table_name),
                 EscapeSqlString(column_name),
                 QuoteIdentifier(table_name),
                 QuoteIdentifier(column_name),
                 QString::number(size));
    }

    return QString("ALTER TABLE \"%1\" ALTER COLUMN \"%2\" TYPE VARCHAR(%3)").arg(table_name, column_name).arg(size);
}

QString Q1MigrationQuery::HasNullDataSQL(QString table_name, QString column_name)
{
    switch (db_type)
    {
    case DatabaseType::SQLServer:
        return QString("SELECT COUNT(*) FROM %1 WHERE %2 IS NULL")
            .arg(QuoteIdentifier(table_name), QuoteIdentifier(column_name));
    case DatabaseType::PostgreSQL:
    case DatabaseType::MySQL:
    case DatabaseType::SQLite:
    default:
        return QString("SELECT COUNT(*) FROM \"%1\" WHERE \"%2\" IS NULL").arg(table_name, column_name);
    }
}

QString Q1MigrationQuery::ColumnProperty(const Q1Column &column) const
{
    if (db_type == DatabaseType::SQLServer)
    {
        if (column.primary_key && Q1Column::IsIdentityDefault(column.default_value))
        {
            return QString("%1 %2 IDENTITY(1,1) PRIMARY KEY")
                .arg(QuoteIdentifier(column.name), ColumnTypeSql(column));
        }

        QStringList parts;
        parts << QuoteIdentifier(column.name);
        parts << ColumnTypeSql(column);
        parts << (column.nullable ? "NULL" : "NOT NULL");

        if (column.primary_key)
            parts << "PRIMARY KEY";

        const QString default_clause = NormalizeDefaultValue(column);
        if (!default_clause.isEmpty())
            parts << QString("DEFAULT %1").arg(default_clause);

        return parts.join(" ");
    }

    QString type = ColumnTypeSql(column);

    if (column.primary_key && Q1Column::IsIdentityDefault(column.default_value))
    {
        QString serial_type = "SERIAL";

        if (column.type == SMALLINT)
            serial_type = "SMALLSERIAL";
        else if (column.type == BIGINT)
            serial_type = "BIGSERIAL";

        return QString("\"%1\" %2 PRIMARY KEY").arg(column.name, serial_type);
    }

    QStringList parts;
    parts << QString("\"%1\"").arg(column.name);
    parts << type;

    if (!column.nullable)
        parts << "NOT NULL";

    if (column.primary_key && !Q1Column::IsIdentityDefault(column.default_value))
        parts << "PRIMARY KEY";

    const QString default_clause = NormalizeDefaultValue(column);
    if (!default_clause.isEmpty())
        parts << QString("DEFAULT %1").arg(default_clause);

    return parts.join(" ");
}

QString Q1MigrationQuery::QuoteIdentifier(const QString &identifier) const
{
    if (db_type == DatabaseType::SQLServer)
    {
        QString escaped = identifier;
        escaped.replace(']', "]]");
        return QString("[%1]").arg(escaped);
    }

    QString escaped = identifier;
    escaped.replace('"', "\"\"");
    return QString("\"%1\"").arg(escaped);
}

QString Q1MigrationQuery::QuoteLiteral(const QString &value) const
{
    QString escaped = value;
    escaped.replace('\'', "''");

    if (db_type == DatabaseType::SQLServer)
        return QString("N'%1'").arg(escaped);

    return QString("'%1'").arg(escaped);
}

QString Q1MigrationQuery::EscapeSqlString(QString value) const
{
    value.replace('\'', "''");
    return value;
}

QString Q1MigrationQuery::ColumnTypeSql(const Q1Column &column) const
{
    const int safe_size = column.size > 0 ? column.size : 255;

    if (db_type == DatabaseType::SQLServer)
    {
        switch (column.type)
        {
        case INTEGER: return "INT";
        case SMALLINT: return "SMALLINT";
        case BIGINT: return "BIGINT";
        case REAL: return "REAL";
        case DOUBLE_PRECISION: return "FLOAT";
        case BOOLEAN: return "BIT";
        case CHAR: return QString("NCHAR(%1)").arg(safe_size);
        case TEXT: return "NVARCHAR(MAX)";
        case VARCHAR: return QString("NVARCHAR(%1)").arg(safe_size);
        case DATE: return "DATE";
        case TIMESTAMP: return "DATETIME2";
        default: return QString("NVARCHAR(%1)").arg(safe_size);
        }
    }

    switch (column.type)
    {
    case INTEGER: return "INTEGER";
    case SMALLINT: return "SMALLINT";
    case BIGINT: return "BIGINT";
    case REAL: return "REAL";
    case DOUBLE_PRECISION: return "DOUBLE PRECISION";
    case BOOLEAN: return "BOOLEAN";
    case CHAR: return QString("CHAR(%1)").arg(safe_size);
    case TEXT: return "TEXT";
    case VARCHAR: return QString("VARCHAR(%1)").arg(safe_size);
    case DATE: return "DATE";
    case TIMESTAMP: return "TIMESTAMP";
    default: return "VARCHAR(255)";
    }
}

QString Q1MigrationQuery::AlterColumnNullabilitySQL(const QString &table_name, const QString &column_name, bool nullable) const
{
    return QString(
               "DECLARE @column_type NVARCHAR(256); "
               "SELECT @column_type = "
               "CASE "
               "WHEN t.name IN ('nvarchar', 'nchar') THEN UPPER(t.name) + '(' + CASE WHEN c.max_length = -1 THEN 'MAX' ELSE CAST(c.max_length / 2 AS NVARCHAR(10)) END + ')' "
               "WHEN t.name IN ('varchar', 'char', 'varbinary', 'binary') THEN UPPER(t.name) + '(' + CASE WHEN c.max_length = -1 THEN 'MAX' ELSE CAST(c.max_length AS NVARCHAR(10)) END + ')' "
               "WHEN t.name IN ('decimal', 'numeric') THEN UPPER(t.name) + '(' + CAST(c.precision AS NVARCHAR(10)) + ',' + CAST(c.scale AS NVARCHAR(10)) + ')' "
               "WHEN t.name IN ('datetime2', 'datetimeoffset', 'time') THEN UPPER(t.name) + '(' + CAST(c.scale AS NVARCHAR(10)) + ')' "
               "ELSE UPPER(t.name) "
               "END "
               "FROM sys.columns c "
               "JOIN sys.types t ON c.user_type_id = t.user_type_id "
               "WHERE c.object_id = OBJECT_ID(N'%1') AND c.name = N'%2'; "
               "IF @column_type IS NOT NULL "
               "EXEC(N'ALTER TABLE %3 ALTER COLUMN %4 ' + @column_type + N' %5');")
        .arg(EscapeSqlString(table_name),
             EscapeSqlString(column_name),
             QuoteIdentifier(table_name),
             QuoteIdentifier(column_name),
             nullable ? "NULL" : "NOT NULL");
}

QString Q1MigrationQuery::DropDefaultConstraintSQL(const QString &table_name, const QString &column_name) const
{
    return QString(
               "DECLARE @constraint_name NVARCHAR(128); "
               "SELECT @constraint_name = dc.name "
               "FROM sys.default_constraints dc "
               "JOIN sys.columns c ON c.default_object_id = dc.object_id "
               "WHERE dc.parent_object_id = OBJECT_ID(N'%1') AND c.name = N'%2'; "
               "IF @constraint_name IS NOT NULL "
               "EXEC(N'ALTER TABLE %3 DROP CONSTRAINT ' + QUOTENAME(@constraint_name));")
        .arg(EscapeSqlString(table_name),
             EscapeSqlString(column_name),
             QuoteIdentifier(table_name));
}

QString Q1MigrationQuery::NormalizeDefaultValue(const Q1Column &column) const
{
    if (column.default_value.isEmpty() || Q1Column::IsIdentityDefault(column.default_value))
        return QString();

    const QString normalized = Q1Column::NormalizeDefaultValue(column.default_value);
    const QString lowered = normalized.toLower();
    const bool numeric_type = column.type == INTEGER || column.type == SMALLINT || column.type == BIGINT ||
                              column.type == REAL || column.type == DOUBLE_PRECISION;

    if (column.type == BOOLEAN)
    {
        if (lowered == "true")
            return db_type == DatabaseType::SQLServer ? "1" : "true";
        if (lowered == "false")
            return db_type == DatabaseType::SQLServer ? "0" : "false";
        return normalized;
    }

    if (numeric_type)
        return normalized;

    if (normalized.startsWith('\'') || normalized.startsWith('"') ||
        normalized.startsWith("N'", Qt::CaseInsensitive))
        return normalized;

    if (normalized.contains('(') && normalized.contains(')'))
        return normalized;

    return QuoteLiteral(normalized);
}

QString Q1MigrationQuery::FormatDefaultExpression(const QString &default_value) const
{
    const QString normalized = Q1Column::NormalizeDefaultValue(default_value);
    if (normalized.isEmpty())
        return QString();

    const QString lowered = normalized.toLower();
    bool is_number = false;
    normalized.toDouble(&is_number);

    if (is_number)
        return normalized;

    if (lowered == "true")
        return db_type == DatabaseType::SQLServer ? "1" : "true";

    if (lowered == "false")
        return db_type == DatabaseType::SQLServer ? "0" : "false";

    if (normalized.startsWith('\'') || normalized.startsWith('"') ||
        normalized.startsWith("N'", Qt::CaseInsensitive))
        return normalized;

    if (normalized.contains('(') && normalized.contains(')'))
        return normalized;

    return QuoteLiteral(normalized);
}
