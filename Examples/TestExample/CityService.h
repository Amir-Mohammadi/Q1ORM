#pragma once

#include "ApplicationContext.h"

class CityService
{
public:
    explicit CityService(ApplicationContext& context) : context(context) {}

    bool Create(City& city)
    {
        error.clear();
        city.name = city.name.trimmed();
        if (city.name.isEmpty() || city.population < 0)
        {
            error = "A city needs a name and a non-negative population.";
            return false;
        }
        if (!context.countries.Select().Where("id", Q1Operator::Equal, city.country_id).Any())
        {
            error = "Country does not exist.";
            return false;
        }
        if (!context.cities.Insert(city))
        {
            error = context.cities.GetLastError();
            return false;
        }
        return true;
    }

    QList<City> InCountry(int countryId)
    {
        return context.cities.Select()
            .Where("country_id", Q1Operator::Equal, countryId)
            .OrderBy("name", Q1Sort::Ascending).ToList();
    }

    QString LastError() const { return error; }

private:
    ApplicationContext& context;
    QString error;
};
