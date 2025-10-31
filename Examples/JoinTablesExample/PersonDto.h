#ifndef PERSONDTO_H
#define PERSONDTO_H

#include <QString>

class PersonDto
{
public:
    int id;
    QString first_name;
    QString last_name;
    int age;
    int address_id;


    static QString TableName() { return "persons"; }

};

#endif // PERSONDTO_H
