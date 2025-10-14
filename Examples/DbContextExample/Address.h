#ifndef ADDRESS_H
#define ADDRESS_H


#include "AddressDto.h"
#include <Q1Core/Q1Entity/Q1Entity.h>
#include <QString>

class Address : public Q1Entity<AddressDto>
{

public:
    explicit Address(Q1Connection* connection = nullptr) : Q1Entity<AddressDto>(connection) {}


    static void ConfigureEntity(Q1Entity<Address> &entity) {
        entity.ToTableName(entity.TableName());
        entity.Property(entity.id, "id", false, true);
        entity.Property(entity.address_name, "address_name", true);
        entity.Property(entity.person_id, "person_id", true);
    }

};

#endif // ADDRESS_H
