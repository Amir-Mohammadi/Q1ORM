#ifndef OBJECT_H
#define OBJECT_H

#include "Person.h"
#include <QString>

class Object
{
public:
    int id;
    int person_id;
    QString name;
    int width;
    int height;
    float weight;

    Person person;
};

#endif // OBJECT_H
