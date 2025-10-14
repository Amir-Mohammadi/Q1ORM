#ifndef OBJECT_H
#define OBJECT_H

#include "ObjectDto.h"
#include <Q1Core/Q1Entity/Q1Entity.h>
#include <QString>

class Object : public Q1Entity<ObjectDto>
{
public:
    explicit Object(Q1Connection* conn = nullptr) : Q1Entity<ObjectDto>(conn) {}

    static void ConfigureEntity(Q1Entity<Object> &entity)
    {
        entity.ToTableName(entity.TableName());
        entity.Property(entity.id, "id", false, true);
        entity.Property(entity.person_id, "person_id", true);
        entity.Property(entity.name, "name", true);
        entity.Property(entity.width, "width", true);
        entity.Property(entity.height, "height", true);
        entity.Property(entity.weight, "weight", false, false, "60");
    };


};
#endif // OBJECT_H




