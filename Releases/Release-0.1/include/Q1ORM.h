#ifndef Q1ORM_H
#define Q1ORM_H

#include <QDebug>

#include "Q1ORM_global.h"
#include "Q1Core/Q1Context/Q1Connection.h"
#include "Q1Core/Q1Context/Q1Context.h"
#include "Q1Core/Q1Entity/Q1Entity.h"
#include "Q1Core/Q1Query/Q1Query.h"
#include "Q1DatabaseInstall/Q1DatabaseInstall.h"

template<typename Entity>
using Q1DbSet = Q1Entity<Entity>;

class Q1ORM_EXPORT Q1ORM
{
public:
    Q1ORM();

//    Q1Context dbContext;
};

#endif // Q1ORM_H
