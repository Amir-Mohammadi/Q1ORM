#ifndef USERMAP_H
#define USERMAP_H

#include "../Models/User.h"
#include <Q1Core/Q1Entity/Q1Entity.h>

class UserMap : public Q1Entity<User>
{
public :
    static void ConfigureEntity(Q1Entity<User>& entity)
    {
        entity.ToTableName("users");
        entity.Property(entity.id, "id", false, true, "GENERATED ALWAYS AS IDENTITY");
        entity.Property(entity.UserName, "UserName", false, false);
        entity.Property(entity.Age, "Age", false, false);
        entity.Property(entity.court_id, "court_id", false, false);
    }


    static QList<Q1Relation> CreateRelations(Q1Entity<User>& entity)
    {
        QList<Q1Relation> relations;

        relations.append(entity.Relations("users", "courts", MANY_TO_ONE, "court_id", "id"));

        return relations;
    }

};


#endif // USERMAP_H
