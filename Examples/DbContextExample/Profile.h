#ifndef PROFILE_H
#define PROFILE_H


#include "Person.h"
#include <QString>

class Profile
{
public:
    int id;
    QString user_name;
    int person_id;

    Person person;

    static QString TableName() { return "Profiles"; }
};

#endif // PROFILE_H
