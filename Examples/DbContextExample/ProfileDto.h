#ifndef PROFILEDTO_H
#define PROFILEDTO_H


#include <QString>

class ProfileDto
{
public:
    int id;
    QString user_name;
    int person_id;

    static QString TableName() { return "profiles"; }
};


#endif // PROFILEDTO_H
