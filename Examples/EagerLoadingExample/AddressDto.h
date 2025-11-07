#ifndef ADDRESSDTO_H
#define ADDRESSDTO_H

#include <QString>

class AddressDto
{
public:
    int id = 0;
    QString city;
    QString street;
    int person_id = 0; // foreign key to Person
};

#endif // ADDRESSDTO_H
