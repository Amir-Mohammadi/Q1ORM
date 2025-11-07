#ifndef OBJECT_H
#define OBJECT_H

#include "ObjectDto.h"
#include <QString>
#include <Q1Core/Q1Entity/Q1Entity.h>
#include <Q1Core/Q1Entity/Q1Relation.h>

class Object : public Q1Entity<ObjectDto>
{
public:
    explicit Object(Q1Connection* connection = nullptr)
        : Q1Entity<ObjectDto>(connection) {}

    static void ConfigureEntity(Q1Entity<ObjectDto>& entity)
    {
        entity.ToTableName(entity.TableName());
        entity.Property(entity.id, "id", false, true);
        entity.Property(entity.person_id, "person_id", true);
        entity.Property(entity.name, "name", true);
    }

    static QList<Q1Relation> CreateRelations(Q1Entity<Object>& entity)
    {
        QList<Q1Relation> relations;

        // Many objects belong to one person
        relations.append(entity.Relations("objects", "persons", MANY_TO_ONE, "person_id", "id"));

        return relations;
    }
};

#endif // OBJECT_H
