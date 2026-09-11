

#include "Q1Core/Q1Context/Q1Connection.h"
#include "Q1Core/Q1Context/Q1Context.h"
#include "Q1Core/Q1Entity/Q1Entity.h"
#include "Q1Core/Q1Entity/Q1Relation.h"
#include "qdebug.h"
// #include <Q1Core/Q1ModelBuilder/Q1ModelBuilder.h>
#include <Q1ORM.h>
#include <qcoreapplication.h>

struct Country {
  int id = 0;
  QString name;
};

struct City {
  int id = 0;
  QString name;
  int country_id = 0;
};

class CountryMap {
public:
  static void ConfigureEntity(Q1Entity<Country> &entity) {
    entity.ToTableName("countries");
    entity.HasKey<&Country::id>().ValueGeneratedOnAdd();
    entity.Property<&Country::name>().IsRequired();
  }
};

class CityMap {
public:
  static void ConfigureEntity(Q1Entity<City> &entity) {
    entity.ToTableName("cities");
    entity.HasKey<&City::id>().ValueGeneratedOnAdd();
    entity.Property<&City::name>().IsRequired();
    entity.Property<&City::country_id>();
    entity.HasOne<Country>().WithMany()
        .HasForeignKey<&City::country_id>()
        .HasPrincipalKey<&Country::id>();
  }
};

class ApplicationDbContext : public Q1Context {
public:
  explicit ApplicationDbContext(Q1Connection *options) {
    SetConnection(options, false);
    RegisterEntity(&cities);
    RegisterEntity(&countries);
  }

  Q1Entity<City> cities;
  Q1Entity<Country> countries;

protected:
  void OnModelCreating(Q1ModelBuilder &builder) override {
    builder.ApplyMap<CountryMap>();
    builder.ApplyMap<CityMap>();
  }
};

int main(int argc, char *argv[]) {
  QCoreApplication app(argc, argv);

  Q1Connection conn(Q1Driver::SQLITE, "", "q1orm_test2.sqlite", "", "", 0);

  ApplicationDbContext ctx(&conn);

  ctx.Initialize();

  Country country;
  country.name = "Germany";
  ctx.countries.Insert(country);

  City city;
  city.name = "Hamburg";
  city.country_id = country.id;
  ctx.cities.Insert(city);

  ctx.countries.Select().ShowList();
  ctx.countries.Select().ShowJson();
  auto all_country = ctx.countries.SelectAll();

  for (const Country &c : all_country) {
    qDebug() << c.name;
  }
}
