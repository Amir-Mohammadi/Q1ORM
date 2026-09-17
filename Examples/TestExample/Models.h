#pragma once

#include <QString>

struct Country
{
    int id = 0;
    QString name;
};

struct City
{
    int id = 0;
    QString name;
    int country_id = 0;
    int population = 0;
};
