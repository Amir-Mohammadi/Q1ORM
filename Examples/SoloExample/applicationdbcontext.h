#ifndef APPLICATIONDBCONTEXT_H
#define APPLICATIONDBCONTEXT_H

#include "Models/City.h"
#include "Models/Country.h"
#include <Q1Core/Q1Context/Q1Context.h>
#include <Q1Core/Q1Entity/Q1Entity.h>


class ApplicationDbContext : public Q1Context
{
public:
    explicit ApplicationDbContext(Q1Connection* conn);

    void OnConfiguration();
    QList<Q1Table> OnTablesCreating();
    QList<Q1Relation> OnTableRelationCreating();


public:
    Q1Entity<City> cities;
    Q1Entity<Country> countries;

};

#endif // APPLICATIONDBCONTEXT_H
