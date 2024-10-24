#ifndef Q1COLUMN_H
#define Q1COLUMN_H

#include <QString>
#include <QStringList>

#include "Q1ORM_global.h"

enum Q1ColumnDataType
{
    INTEGER,                    // int
    SMALLINT,                   // short
    BIGINT,                     // long
    REAL,                       // float
    DOUBLE_PRECISION,           // double
    BOOLEAN,                    // bool
    CHAR,                       // char
    TEXT,                       // QString
    VARCHAR,                    // char[]
    DATE,                       // QDate
    TIMESTAMP_WITHOUT_TIMEZONE  // QDateTime
};

class Q1ORM_EXPORT Q1Column
{
public:
    Q1Column(QString name,
             Q1ColumnDataType type,
             int size,
             bool nullable,
             bool primary_key,
             QString default_value)
    {
        this->name = name;
        this->type = type;
        this->size = size;
        this->nullable = nullable;
        this->primary_key = primary_key;
        this->default_value = default_value;
    }

    QString name;
    Q1ColumnDataType type;
    int size;
    bool nullable;
    bool primary_key;
    QString default_value;

public:
    bool operator==(const Q1Column& q1column) const
    {
        return this->name == q1column.name &&
               this->type == q1column.type &&
               this->primary_key == q1column.primary_key;
    }

    static int IndexOf(const QList<Q1Column>& q1columns, const Q1Column& q1column)
    {
        int index = -1;

        for(int i = 0; i < q1columns.count(); i++)
        {
            if(q1columns[i] == q1column)
            {
                index = i;
                break;
            }
        }

        return index;
    }

public:
    static Q1ColumnDataType GetVariableType(QString type)
    {
        QStringList data_types = {"int",
                                  "short",
                                  "long",
                                  "float",
                                  "double",
                                  "bool",
                                  "char",
                                  "class QString",
                                  "char []",
                                  "class QDate",
                                  "class QDateTime"};

        int index_of = data_types.indexOf(type);

        return Q1ColumnDataType(index_of);
    }

    static Q1ColumnDataType GetColumnType(QString type)
    {
        QStringList data_types = {"integer",
                                  "smallint",
                                  "bigint",
                                  "real",
                                  "double precision",
                                  "boolean",
                                  "character",
                                  "text",
                                  "character varying",
                                  "date",
                                  "timestamp without time zone"};

        int index_of = data_types.indexOf(type);

        return Q1ColumnDataType(index_of);
    }
};

#endif // Q1COLUMN_H
