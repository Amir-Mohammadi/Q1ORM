#ifndef OBJECTDTO_H
#define OBJECTDTO_H


#include <QString>

class ObjectDto
{
public:
    int id;
    int person_id;
    QString name;
    int width;
    int height;
    float weight;


    static QString TableName() { return "objects"; }
};


#endif // OBJECTDTO_H
