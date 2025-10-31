#include "PersonDto.h"
#include <Q1Core/Q1Entity/Q1Entity.h>


class Person : public Q1Entity<PersonDto>
{
public:
    explicit Person(Q1Connection* connection = nullptr) : Q1Entity<PersonDto>(connection) { ConfigureEntity(*this); }


    static void ConfigureEntity(Q1Entity<PersonDto> &entity)
    {

        entity.ToTableName("persons");

        entity.Property(entity.id, "id", false, true, "");
        entity.Property(entity.first_name, "first_name", true);
        entity.Property(entity.last_name, "last_name", true);
        entity.Property(entity.age, "age", false, false);
        entity.Property(entity.address_id, "address_id", true);
    }

};
