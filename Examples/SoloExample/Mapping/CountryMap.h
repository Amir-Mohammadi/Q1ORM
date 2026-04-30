#ifndef COUNTRYMAP_H
#define COUNTRYMAP_H



#include "../Models/Country.h"
#include <Q1Core/Q1Entity/Q1Entity.h>

class CountryMap : public Q1Entity<Country>
{
public :
    static void ConfigureEntity(Q1Entity<Country>& entity)
    {
        entity.ToTableName("countries");
        entity.Property(entity.id, "id", false, true, "GENERATED ALWAYS AS IDENTITY");
        entity.Property(entity.name, "name", false, false);
    }

    static QList<Q1Relation> CreateRelations(Q1Entity<Country>& entity)
    {
        QList<Q1Relation> relations;

        relations.append(entity.Relations("countries", "cities", ONE_TO_MANY, "country_id", "id"));

        return relations;
    }



};


#endif // COUNTRYMAP_H
