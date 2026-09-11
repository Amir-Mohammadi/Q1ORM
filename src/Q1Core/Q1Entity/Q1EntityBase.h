#ifndef Q1ENTITYBASE_H
#define Q1ENTITYBASE_H

#include "../Q1Context/Q1Connection.h"
#include <typeindex>


class Q1EntityBase
{
public:
    virtual ~Q1EntityBase() = default;
    virtual void SetConnection(Q1Connection* conn) = 0;

    virtual std::type_index EntityType() const = 0;
    
};

#endif // Q1ENTITYBASE_H