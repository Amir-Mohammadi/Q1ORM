


#ifndef PERSON_H
#define PERSON_H

#include "PersonDto.h"
#include <Q1Core/Q1Entity/Q1Entity.h>

class Person : public Q1Entity<PersonDto>
{
public:
    explicit Person(Q1Connection* connection = nullptr)
        : Q1Entity<PersonDto>(connection)
    {
        ConfigureEntity(*this);
        CreateRelations(*this);
    }

    // Configure entity properties
    static void ConfigureEntity(Q1Entity<PersonDto>& entity)
    {
        entity.ToTableName("persons");
        entity.Property(entity.id, "id", false, true, "");
        entity.Property(entity.first_name, "first_name", true);
        entity.Property(entity.last_name, "last_name", true);
        entity.Property(entity.age, "age", false, false);
        entity.Property(entity.address_id, "address_id", true);
    }

    // Define relationships for Include()
    static void CreateRelations(Q1Entity<PersonDto>& entity)
    {
        // ONE_TO_MANY: One person can have one address
        // base_table: "persons" (this table)
        // top_table: "addresses" (related table)
        // foreign_key: "address_id" (column in persons that references addresses)
        // reference_key: "id" (primary key in addresses table)
        entity.Relations("persons", "addresses", ONE_TO_MANY, "address_id", "id");

        // Add more relations here if needed:
        // entity.Relations("persons", "cities", ONE_TO_MANY, "city_id", "id");
    }
};

#endif // PERSON_H
