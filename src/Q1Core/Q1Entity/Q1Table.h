#ifndef Q1TABLE_H
#define Q1TABLE_H

#include <QStringlist>

#include "Q1Core/Q1Entity/Q1Column.h"
#include "Q1Core/Q1Entity/Q1Relation.h"

#include "Q1ORM_global.h"

class Q1ORM_EXPORT Q1Table
{
public:
    QString table_name;
    QList<Q1Column> columns;
    QList<Q1Relation> relations;
};

#endif // Q1ENTITY_H
