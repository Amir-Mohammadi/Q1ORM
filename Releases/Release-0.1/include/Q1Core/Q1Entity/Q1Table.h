#ifndef Q1TABLE_H
#define Q1TABLE_H

#include <QStringlist>

#include "../../Q1Core/Q1Entity/Q1Column.h"
#include "../../Q1Core/Q1Entity/Q1Relation.h"

#include "../../Q1ORM_global.h"

class Q1ORM_EXPORT Q1Table
{

public:
    Q1Table() {}

    Q1Table(const QString& name) : table_name(name) {}

    void AddColumn(const Q1Column& column)
    {
        columns.append(column);
    }

    void AddRelation(const Q1Relation& relation)
    {
        relations.append(relation);
    }

    Q1Column* FindColumn(const QString& name)
    {
        for(int i = 0; i < columns.size(); i++)
        {
            if(columns[i].name.toLower() == name.toLower())
                return &columns[i];
        }

        return nullptr;
    }

    bool HasColumn(const QString& name) const
    {
        for(const Q1Column& col : columns)
        {
            if(col.name.toLower() == name.toLower())
                return true;
        }

        return false;
    }

    bool HasForeignKey() const
    {
        return !relations.isEmpty();
    }

    QList<Q1Column> GetPrimaryKeys() const
    {
        QList<Q1Column> pk_columns;
        for (const Q1Column& col : columns)
        {
            if (col.primary_key)
                pk_columns.append(col);
        }
        return pk_columns;
    }


    bool IsValid() const
    {
        return !table_name.isEmpty() && !columns.isEmpty();
    }


public:
    QString table_name;
    QList<Q1Column> columns;
    QList<Q1Relation> relations;
};

#endif // Q1ENTITY_H
