#ifndef APPLICATIONDBCONTEXT_H
#define APPLICATIONDBCONTEXT_H

#include "Models/City.h"
#include "Models/Club.h"
#include "Models/Country.h"
#include "Models/Court.h"
#include "Models/User.h"

#include <Q1Core/Q1Context/Q1Context.h>
#include <Q1Core/Q1Entity/Q1Entity.h>

class ApplicationDbContext : public Q1Context
{
public:
    explicit ApplicationDbContext(Q1Connection* conn);

    void OnConfiguration() override;
    QList<Q1Table*> OnTablesCreating() override;
    QList<Q1Relation> OnTableRelationCreating() override;

public:
    Q1Entity<City> cities;
    Q1Entity<Club> clubs;
    Q1Entity<Country> countries;
    Q1Entity<Court> courts;
    Q1Entity<User> users;
};

#endif // APPLICATIONDBCONTEXT_H