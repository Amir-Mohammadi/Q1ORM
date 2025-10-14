#ifndef ADDRESSDTO_H
#define ADDRESSDTO_H

#include <QString>

class AddressDto
{

public:
    int id;
    QString address_name;
    int person_id;


    static QString TableName() { return "addresses"; }
};

#endif // ADDRESSDTO_H
