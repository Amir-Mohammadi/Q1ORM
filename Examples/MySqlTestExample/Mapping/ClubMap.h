#ifndef CLUBMAP_H
#define CLUBMAP_H

#include "../Models/Club.h"
#include <Q1Core/Q1Entity/Q1Entity.h>

class ClubMap : public Q1Entity<Club>
{
public :
    static void ConfigureEntity(Q1Entity<Club>& entity)
    {
        entity.ToTableName("clubs");
        entity.Property(entity.id, "id", false, true, "GENERATED ALWAYS AS IDENTITY");
        entity.Property(entity.name, "name", false, false);
        entity.Property(entity.city_id, "city_id", false, false);
    }


    static QList<Q1Relation> CreateRelations(Q1Entity<Club>& entity)
    {
        QList<Q1Relation> relations;

        relations.append(entity.Relations("clubs", "cities", MANY_TO_ONE, "city_id", "id"));
        relations.append(entity.Relations("clubs", "courts", ONE_TO_MANY, "club_id", "id"));

        return relations;
    }

};


#endif // CLUBMAP_H
