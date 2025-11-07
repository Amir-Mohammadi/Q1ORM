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



};


#endif // COUNTRYMAP_H
