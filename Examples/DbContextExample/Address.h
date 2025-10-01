#ifndef ADDRESS_H
#define ADDRESS_H


#include "Person.h"
#include <QString>

class Address
{

public:
    int id;
    QString address_name;
    int person_id;

    Person person;

    static QString TableName() { return "Addresses"; }
};

#endif // ADDRESS_H
