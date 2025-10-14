#ifndef PERSON_H
#define PERSON_H

#include "Address.h"
#include "Object.h"
#include "PersonDto.h"
#include "Profile.h"
#include <QString>

#include <Q1Core/Q1Entity/Q1Entity.h>



class Person : public Q1Entity<PersonDto>
{
public:
    explicit Person(Q1Connection* connection = nullptr) : Q1Entity<PersonDto>(connection) {}


    static void ConfigureEntity(Q1Entity<Person> &entity)
    {
        entity.ToTableName(entity.TableName());

        entity.Property(entity.id, "id", false, true);
        entity.Property(entity.first_name, "first_name", true);
        entity.Property(entity.last_name, "last_name", true);
        entity.Property(entity.age, "age", false, false);
        entity.Property(entity.address_id, "address_id", true);
    }


    static QList<Q1Relation> CreateRelations(Q1Entity<Person> &persons,
                                             Q1Entity<Address> &addresses,
                                             Q1Entity<Object> &objects,
                                             Q1Entity<Profile> &profiles)
    {
        QList<Q1Relation> relations;


            // addresses.person_id -> persons.id (ONE_TO_MANY)
            relations.append(addresses.Relations(addresses.TableName(),
                                                 persons.TableName(),
                                                 Q1RelationType::ONE_TO_MANY,
                                                 "person_id",
                                                 "id"));


           //object.person_id -> persons.id   (ONE_TO_MANY)
           relations.append(objects.Relations(objects.TableName(),
                                              persons.TableName(),
                                              Q1RelationType::ONE_TO_MANY,
                                              "person_id",
                                              "id"));



            // profiles.person_id -> persons.id  (ONE_TO_ONE)
            relations.append(profiles.Relations(profiles.TableName(),
                                                persons.TableName(),
                                                Q1RelationType::ONE_TO_ONE,
                                                "person_id",
                                                "id"));


            return relations;

    }
};

#endif // PERSON_H
