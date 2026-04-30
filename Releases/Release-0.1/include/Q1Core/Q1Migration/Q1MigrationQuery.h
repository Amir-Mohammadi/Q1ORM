#ifndef Q1MIGRATIONQUERY_H
#define Q1MIGRATIONQUERY_H

#include <QList>
#include <QDebug>

#include "../../Q1Core/Q1Entity/Q1Column.h"
#include "../../Q1Core/Q1Entity/Q1Table.h"
#include "../../Q1Core/Q1Entity/Q1Relation.h"

enum class DatabaseType
{
    PostgreSQL,
    SQLServer,
    MySQL,
    SQLite
};

class Q1ORM_EXPORT Q1MigrationQuery
{
public:
    Q1MigrationQuery(DatabaseType type = DatabaseType::PostgreSQL);

    QString GetDatabasesSQL();
    QString GetColumnsSQL(QString table_name);
    QString ConstraintExistsSQL(const QString &constraint_name);

    QString AddDatabaseSQL(QString database_name);
    QString AddTableSQL(Q1Table &q1table);
    QString AddColumnSQL(QString table_name, const Q1Column &column);
    QString AddRelationSQL(const Q1Relation &relation);

    QString DropTableSQL(QString table_name);
    QString DropColumnSQL(QString table_name, QString column_name);
    QString DropColumnNullableSQL(QString table_name, QString column_name);
    QString DropColumnDefaultSQL(QString table_name, QString column_name);

    QString SetColumnNullableSQL(QString table_name, QString column_name);
    QString SetColumnDefaultSQL(QString table_name, QString column_name, QString default_value);

    QString UpdateColumnSizeSQL(QString table_name, QString column_name, int size);

    QString HasNullDataSQL(QString table_name, QString column_name);

    QString lastError() const { return m_lastError; }

private:
    QString ColumnProperty(const Q1Column &column) const;
    QString QuoteIdentifier(const QString &identifier) const;
    QString QuoteLiteral(const QString &value) const;
    QString EscapeSqlString(QString value) const;
    QString ColumnTypeSql(const Q1Column &column) const;
    QString AlterColumnNullabilitySQL(const QString &table_name, const QString &column_name, bool nullable) const;
    QString DropDefaultConstraintSQL(const QString &table_name, const QString &column_name) const;
    QString NormalizeDefaultValue(const Q1Column &column) const;
    QString FormatDefaultExpression(const QString &default_value) const;
    QString m_lastError;
    DatabaseType db_type;
};

#endif // Q1MIGRATIONQUERY_H
