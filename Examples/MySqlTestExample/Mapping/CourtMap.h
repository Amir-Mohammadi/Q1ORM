#ifndef COURTMAP_H
#define COURTMAP_H

#include "../Models/Court.h"
#include <Q1Core/Q1Entity/Q1Entity.h>

class CourtMap : public Q1Entity<Court>
{
public :
    static void ConfigureEntity(Q1Entity<Court>& entity)
    {
        entity.ToTableName("courts");
        entity.Property(entity.id, "id", false, true, "GENERATED ALWAYS AS IDENTITY");
        entity.Property(entity.name, "name", false, false);
        entity.Property(entity.club_id, "club_id", false, false);
    }


    static QList<Q1Relation> CreateRelations(Q1Entity<Court>& entity)
    {
        QList<Q1Relation> relations;

        relations.append(entity.Relations("courts", "clubs", MANY_TO_ONE, "club_id", "id"));
        relations.append(entity.Relations("courts", "users", ONE_TO_MANY, "court_id", "id"));

        return relations;
    }

};


#endif // COURTMAP_H
