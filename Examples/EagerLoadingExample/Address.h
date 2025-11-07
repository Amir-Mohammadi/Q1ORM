#ifndef ADDRESS_H
#define ADDRESS_H

#include "AddressDto.h"
#include <Q1Core/Q1Entity/Q1Entity.h>
#include <Q1Core/Q1Entity/Q1Relation.h>

class Address : public Q1Entity<AddressDto>
{
public:
    explicit Address(Q1Connection* connection = nullptr)
        : Q1Entity<AddressDto>(connection) {}

    static void ConfigureEntity(Q1Entity<AddressDto>& entity)
    {
        entity.ToTableName("addresses");
        entity.Property(entity.id, "id", false, true);
        entity.Property(entity.person_id, "person_id", false, false);
    }

    static QList<Q1Relation> CreateRelations(Q1Entity<AddressDto>& entity)
    {
        QList<Q1Relation> relations;
        relations.append(entity.Relations("addresses", "persons", MANY_TO_ONE, "person_id", "id"));

        return relations;
    }
};

#endif // ADDRESS_H
