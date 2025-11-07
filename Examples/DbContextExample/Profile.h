#ifndef PROFILE_H
#define PROFILE_H

#include "ProfileDto.h"
#include <QString>
#include <Q1Core/Q1Entity/Q1Entity.h>
#include <Q1Core/Q1Entity/Q1Relation.h>

class Profile : public Q1Entity<ProfileDto>
{
public:
    explicit Profile(Q1Connection* connection = nullptr)
        : Q1Entity<ProfileDto>(connection) {}

    static void ConfigureEntity(Q1Entity<ProfileDto>& entity)
    {
        entity.ToTableName(entity.TableName());
        entity.Property(entity.id, "id", false, true);
        entity.Property(entity.user_name, "user_name", true);
        entity.Property(entity.person_id, "person_id", true);
    }

    static QList<Q1Relation> CreateRelations(Q1Entity<Profile>& entity)
    {
        QList<Q1Relation> relations;

        // Each profile belongs to one person
        relations.append(entity.Relations("profiles", "persons", ONE_TO_ONE, "person_id", "id"));

        return relations;
    }
};

#endif // PROFILE_H
