#ifndef OBJECTDTO_H
#define OBJECTDTO_H

#include <QString>

class ObjectDto
{
public:
    int id = 0;
    int person_id = 0; // FK -> persons.id
    QString name;

      static QString TableName() { return "objects"; }
};

#endif // OBJECTDTO_H
