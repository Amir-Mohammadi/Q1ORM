#ifndef PROFILE_H
#define PROFILE_H



#include "ProfileDto.h"
#include <Q1Core/Q1Entity/Q1Entity.h>
#include <QString>


class Profile : public Q1Entity<ProfileDto>
{
public:
    explicit Profile(Q1Connection* conn = nullptr) : Q1Entity<ProfileDto>(conn) {}

    static void ConfigureEntity(Q1Entity<Profile> &entity)
    {
        entity.ToTableName(entity.TableName());
        entity.Property(entity.id, "id", false, true, "GENERATED ALWAYS AS IDENTITY");
        entity.Property(entity.user_name, "user_name", true);
        entity.Property(entity.person_id, "person_id", true);
    }
};

#endif // PROFILE_H
