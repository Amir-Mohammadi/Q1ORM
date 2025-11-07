#ifndef INSERTSERVICE_H
#define INSERTSERVICE_H

#include "../Models/City.h"
#include "../Models/Country.h"
#include "../Repository/Repository.h"

class InsertService
{
public:
    InsertService(Q1Entity<City>& citiesEntity,
                  Q1Entity<Country>& countriesEntity)
        : cityRepo(citiesEntity),
        countryRepo(countriesEntity) {}

    // --- CRUD ---
    bool AddCity(City& city) { return cityRepo.Insert(city); }
    bool AddCountry(Country& country) { return countryRepo.Insert(country); }

    bool UpdateCity(City& city, const QString& where) { return cityRepo.Update(city, where); }
    bool UpdateCountry(Country& country, const QString& where) { return countryRepo.Update(country, where); }

    bool DeleteCity(int id) { return cityRepo.Delete(id); }
    bool DeleteCountry(int id) { return countryRepo.Delete(id); }

    City GetCityById(int id) { return cityRepo.SelectById(id); }
    Country GetCountryById(int id) { return countryRepo.SelectById(id); }

    // --- Select ---
    QList<City> GetAllcities() { return cityRepo.SelectAll(); }
    QList<Country> GetAllcountries() { return countryRepo.SelectAll(); }

    QList<Country> GetcountriesWithcities()
    {
        return countryRepo.SelectJoin("cities", "cities.country_id = countries.id");
    }

    QList<Country> GetcountriesIncludecities()
    {
        return countryRepo.SelectInclude({"cities"});
    }

private:
    Repository<City> cityRepo;
    Repository<Country> countryRepo;
};

#endif // INSERTSERVICE_H
