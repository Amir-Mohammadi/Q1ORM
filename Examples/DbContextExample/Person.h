#ifndef PERSON_H
#define PERSON_H

#include <QString>

class Person
{
public:
    int id;
    QString first_name;
    QString last_name;
    int age;
    int address_id;

    static QString TableName() { return "Persons"; }
};

#endif // PERSON_H
