#ifndef PROFILEDTO_H
#define PROFILEDTO_H

#include <QString>

class ProfileDto
{
public:
    int id = 0;
    QString user_name;
    int person_id = 0; // FK -> persons.id

    static QString TableName() { return "profiles"; }
};

#endif // PROFILEDTO_H
