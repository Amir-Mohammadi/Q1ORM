#ifndef Q1RELATION_H
#define Q1RELATION_H


#include <QString>

#include "Q1ORM_global.h"

enum Q1RelationType
{
    ONE_TO_ONE,
    ONE_TO_MANY,
    MANY_TO_ONE,
    MANY_TO_MANY
};

class Q1ORM_EXPORT Q1Relation
{
public:
    Q1Relation() { }

    QString foreign_key;
    QString base_table;
    QString top_table;
    Q1RelationType type;

};

#endif // Q1RELATION_H
