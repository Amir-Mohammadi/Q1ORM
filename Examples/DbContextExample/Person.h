#ifndef PERSON_H
#define PERSON_H

#include "PersonDto.h"
#include <QString>
#include <Q1Core/Q1Entity/Q1Entity.h>
#include <Q1Core/Q1Entity/Q1Relation.h>

class Person : public Q1Entity<PersonDto>
{
public:
    explicit Person(Q1Connection* connection = nullptr)
        : Q1Entity<PersonDto>(connection) {}

    static void ConfigureEntity(Q1Entity<PersonDto>& entity)
    {
        entity.ToTableName(entity.TableName());
        entity.Property(entity.id, "id", false, true);
        entity.Property(entity.first_name, "first_name", true);
        entity.Property(entity.last_name, "last_name", true);
        entity.Property(entity.age, "age", false, false);
    }

    static QList<Q1Relation> CreateRelations(Q1Entity<Person>& entity)
    {
        QList<Q1Relation> relations;

        // One-to-one with profile
        // relations.append(entity.Relations("profile", "profiles", ONE_TO_ONE, "id", "person_id"));

        // // One-to-many with addresses
        // relations.append(entity.Relations("addresses", "addresses", ONE_TO_MANY, "id", "person_id"));

        // // One-to-many with objects
        // relations.append(entity.Relations("objects", "objects", ONE_TO_MANY, "id", "person_id"));

        return relations;
    }
};

#endif // PERSON_H
