#ifndef Q1COLUMN_H
#define Q1COLUMN_H

#include <QString>
#include <QStringList>
#include <QRegularExpression>

#include "../../Q1ORM_global.h"



enum Q1ColumnDataType
{
    INTEGER = 0,
    SMALLINT = 1,
    BIGINT = 2,
    REAL = 3,
    DOUBLE_PRECISION = 4,
    BOOLEAN = 5,
    CHAR = 6,
    TEXT = 7,
    VARCHAR = 8,
    DATE = 9,
    TIMESTAMP = 10
};


class Q1ORM_EXPORT Q1Column
{
public:
    Q1Column() {}

    inline Q1Column(QString name,
             Q1ColumnDataType type,
             int size = 0,
             bool nullable = true,
             bool primary_key = false,
             QString default_value = QString(),
             bool is_identity = false):
        name(name),
        type(type),
        size(size),
        nullable(nullable),
        primary_key(primary_key),
        default_value(default_value),
        is_identity(is_identity)
    {
    }




    // Comparison operator for column names (case-insensitive)
    bool operator==(const Q1Column& other) const
    {
        return name.toLower() == other.name.toLower();
    }

    bool operator!=(const Q1Column& other) const
    {
        return !(*this == other);
    }


    // Static helper methods
    static Q1ColumnDataType GetVariableType(const QString& cpp_type)
    {
        // Remove compiler-specific prefixes
        // MSVC adds "class ", GCC adds numbers like "7QString"
        QString clean_type = cpp_type;
        clean_type.remove("class ");
        clean_type.remove("struct ");
        clean_type.remove(QRegularExpression("^\\d+"));
        clean_type = clean_type.trimmed();

        if (clean_type.contains("int") || clean_type.contains("Int") || clean_type == "i")
            return INTEGER;
        else if (clean_type.contains("short") || clean_type.contains("Short") || clean_type == "s")
            return SMALLINT;
        else if (clean_type.contains("long") || clean_type.contains("Long") || clean_type == "l" || clean_type == "x")
            return BIGINT;
        else if (clean_type.contains("float") || clean_type.contains("Float") || clean_type == "f")
            return REAL;
        else if (clean_type.contains("double") || clean_type.contains("Double") || clean_type == "d")
            return DOUBLE_PRECISION;
        else if (clean_type.contains("bool") || clean_type.contains("Bool") || clean_type == "b")
            return BOOLEAN;
        else if (clean_type.contains("QString") || clean_type.contains("String"))
            return VARCHAR;
        else if (clean_type.contains("QDate") && !clean_type.contains("QDateTime"))
            return DATE;
        else if (clean_type.contains("QDateTime") || clean_type.contains("QTime"))
            return TIMESTAMP;
        else if (clean_type.contains("char"))
            return CHAR;

        // Default to VARCHAR for unknown types
        return VARCHAR;
    }

    static Q1ColumnDataType GetColumnType(const QString& sql_type)
    {
        QString type = sql_type.toLower().trimmed();

        if (type == "integer" || type == "int" || type == "int4")
            return INTEGER;
        else if (type == "smallint" || type == "int2")
            return SMALLINT;
        else if (type == "bigint" || type == "int8")
            return BIGINT;
        else if (type == "real" || type == "float4")
            return REAL;
        else if (type == "double precision" || type == "float8")
            return DOUBLE_PRECISION;
        else if (type == "boolean" || type == "bool")
            return BOOLEAN;
        else if (type.startsWith("character(") || type == "char")
            return CHAR;
        else if (type == "text")
            return TEXT;
        else if (type.startsWith("character varying") || type.startsWith("varchar"))
            return VARCHAR;
        else if (type == "date")
            return DATE;
        else if (type.startsWith("timestamp"))
            return TIMESTAMP;

        return VARCHAR;
    }


    static int IndexOf(const QList<Q1Column>& columns, const Q1Column& column)
    {
        for (int i = 0; i < columns.size(); ++i)
        {
            if (columns[i] == column)
                return i;
        }
        return -1;
    }

    static bool Contains(const QList<Q1Column>& columns, const QString& column_name)
    {
        for (const Q1Column& col : columns)
        {
            if (col.name.toLower() == column_name.toLower())
                return true;
        }
        return false;
    }


public:
    QString name;
    Q1ColumnDataType type;
    int size;
    bool nullable;
    bool primary_key;
    QString default_value;
    bool is_identity = false;
};

#endif // Q1COLUMN_H
