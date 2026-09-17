#pragma once

#include <Q1ORM.h>
#include "Models.h"

struct CountryMap
{
    static void ConfigureEntity(Q1Entity<Country>& entity)
    {
        entity.ToTableName("test_countries");
        entity.HasKey<&Country::id>().ValueGeneratedOnAdd();
        entity.Property<&Country::name>().IsRequired();
    }
};

struct CityMap
{
    static void ConfigureEntity(Q1Entity<City>& entity)
    {
        entity.ToTableName("test_cities");
        entity.HasKey<&City::id>().ValueGeneratedOnAdd();
        entity.Property<&City::name>().IsRequired();
        entity.Property<&City::country_id>();
        entity.Property<&City::population>();
        entity.HasOne<Country>().WithMany()
            .HasForeignKey<&City::country_id>()
            .HasPrincipalKey<&Country::id>();
    }
};

class ApplicationContext : public Q1Context
{
public:
    explicit ApplicationContext(Q1Connection& connection)
    {
        SetConnection(&connection, false);
        RegisterEntity(&countries);
        RegisterEntity(&cities);
    }

    Q1Entity<Country> countries;
    Q1Entity<City> cities;

protected:
    void OnModelCreating(Q1ModelBuilder& builder) override
    {
        builder.ApplyMap<CityMap>().ApplyMap<CountryMap>();
    }
};
