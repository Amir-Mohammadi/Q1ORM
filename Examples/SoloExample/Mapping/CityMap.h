#ifndef CITYMAP_H
#define CITYMAP_H

#include "../Models/City.h"
#include <Q1Core/Q1Entity/Q1Entity.h>

class CityMap : public Q1Entity<City>
{
public :
    static void ConfigureEntity(Q1Entity<City>& entity)
    {
        entity.ToTableName("cities");
        entity.Property(entity.id, "id", false, true, "GENERATED ALWAYS AS IDENTITY");
        entity.Property(entity.name, "name", false, false);
        entity.Property(entity.country_id, "country_id", false, false);
    }


    static QList<Q1Relation> CreateRelations(Q1Entity<City>& entity)
    {
        QList<Q1Relation> relations;

        relations.append(entity.Relations("countries", "cities", ONE_TO_MANY, "id", "country_id"));

        return relations;
    }

};


#endif // CITYMAP_H
