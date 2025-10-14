#include <QCoreApplication>
#include "../../src/Q1Core/Q1Context/Q1Context.h"
#include "../../src/Q1Core/Q1Entity/Q1Entity.h"
#include "Address.h"
#include "Person.h"
#include "Object.h"
#include "Profile.h"

class TestContext : public Q1Context
{
public:
    TestContext(Q1Connection* conn) : persons(conn),
                                      objects(conn),
                                      addresses(conn),
                                      profiles(conn)
    { connection = conn; }

    void OnConfiguration() override {
        if (!connection)
            connection = new Q1Connection(Q1Driver::POSTGRE_SQL, "localhost", "testDb", "postgres", "123");
    }


    QList<Q1Table> OnTablesCreating() override {

// //method 1
//         // persons.ToTableName(persons.TableName());
//         // persons.Property(persons.id, "id", false, true);
//         // persons.Property(persons.first_name, "first_name", true);
//         // persons.Property(persons.last_name, "last_name", true);
//         // persons.Property(persons.age, "age", false, false);
//         // persons.Property(persons.address_id, "address_id", true);

//         // objects.ToTableName(objects.TableName());
//         // objects.Property(objects.id, "id", false, true);
//         // objects.Property(objects.person_id, "person_id", true);
//         // objects.Property(objects.name, "name", true);
//         // objects.Property(objects.width, "width", true);
//         // objects.Property(objects.height, "height", true);
//         // objects.Property(objects.weight, "weight", false, false, "60");


//         // addresses.ToTableName(addresses.TableName());
//         // addresses.Property(addresses.id,"id", false, true);
//         // addresses.Property(addresses.address_name, "address_name",true);
//         // addresses.Property(addresses.person_id,"person_id",true);



//         // profiles.ToTableName(profiles.TableName());
//         // profiles.Property(profiles.id,"id", false, true);
//         // profiles.Property(profiles.user_name, "user_name",true);
//         // profiles.Property(profiles.person_id,"person_id",true);



// //method 2
//         Person::ConfigureEntity(persons);
//         Address::ConfigureEntity(addresses);
//         Object::ConfigureEntity(objects);
//         Profile::ConfigureEntity(profiles);

return {};
//         return { persons.GetTable(), addresses.GetTable(), objects.GetTable(), profiles.GetTable()};
    }

    QList<Q1Relation> OnTableRelationCreating() override
    {

//         QList<Q1Relation> relations;


// //method 1
//         // relations.append(objects.Relations(objects.TableName(), persons.TableName(), Q1RelationType::ONE_TO_MANY, "person_id", "id"));
//         // relations.append(addresses.Relations(addresses.TableName(), persons.TableName(), Q1RelationType::ONE_TO_MANY, "person_id", "id"));
//         // relations.append(profiles.Relations (profiles.TableName(), persons.TableName(), Q1RelationType::ONE_TO_ONE, "person_id", "id"));


// //method 1
//         // Let Person expose relations that involve Person and other entities
//         relations.append(Person::CreateRelations(persons, addresses ,objects, profiles));

//         // If other entities have extra relations not covered by Person, append them here:
//         // relations.append(Object::CreateRelations(...));
//         // (Note: CreateRelations returns a QList, append(...) accepts a QList)

//         return relations;
return {};

    }


public:
    Q1Entity<Person> persons;
    Q1Entity<Object> objects;
    Q1Entity<Address> addresses;
    Q1Entity<Profile> profiles;
};

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    Q1Connection* conn = new Q1Connection(Q1Driver::POSTGRE_SQL, "localhost", "testDb", "postgres", "123");

    if (!conn->RootConnect() || !conn->Connect())
        return 1;

    TestContext ctx(conn);
    ctx.Initialize();


    QList<Person> people = ctx.persons.Select();
    qDebug() << "Total rows:" << people.count();


    for (const Person& p : people)
        qDebug() << p.id << p.first_name << p.last_name << p.age << p.address_id;



    conn->Disconnect();
    conn->RootDisconnect();

    return 0;
}
