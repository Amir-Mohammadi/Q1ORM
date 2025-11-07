#ifndef PERSON_H
#define PERSON_H

#include "PersonDto.h"
#include <QString>
#include <Q1Core/Q1Entity/Q1Entity.h>
#include <Q1Core/Q1Entity/Q1Relation.h> // include relation type

class Person : public Q1Entity<PersonDto>
{
public:

    explicit Person(Q1Connection* connection = nullptr) : Q1Entity<PersonDto>(connection) {}

    // ConfigureEntity must accept Q1Entity<PersonDto>&
    static void ConfigureEntity(Q1Entity<PersonDto> &entity)
    {
        entity.ToTableName("persons");
        entity.Property(entity.id, "id", false, true);
        entity.Property(entity.first_name, "first_name", true);
        entity.Property(entity.last_name, "last_name", true);
        entity.Property(entity.age, "age", false, false);
    }

    // If Person has relations to other tables, add CreateRelations here (optional)
    static QList<Q1Relation> CreateRelations(Q1Entity<PersonDto> & /*entity*/)
    {
        return {};
    }
};

#endif // PERSON_H
