#include <QCoreApplication>
#include "../../src/Q1Core/Q1Context/Q1Context.h"
#include "../../src/Q1Core/Q1Entity/Q1Entity.h"
#include "Address.h"
#include "Person.h"
#include "Object.h"
#include "Profile.h"

// Your custom DbContext
class TestContext : public Q1Context
{
public:
    explicit TestContext(Q1Connection* conn)
        : persons(conn), objects(conn), addresses(conn), profiles(conn)
    {
        connection = conn;
    }

    void OnConfiguration() override
    {

        if (!connection)
            connection = new Q1Connection(Q1Driver::POSTGRE_SQL, "localhost", "solo1", "postgres", "123");

    }

    QList<Q1Table> OnTablesCreating() override
    {
        Person::ConfigureEntity(persons);

        Address::ConfigureEntity(addresses);
        Object::ConfigureEntity(objects);
        Profile::ConfigureEntity(profiles);

        return { persons.GetTable(), addresses.GetTable(), objects.GetTable(), profiles.GetTable() };
    }

    QList<Q1Relation> OnTableRelationCreating() override
    {
        QList<Q1Relation> relations;

        // Append all defined entity relationships
        relations.append(Address::CreateRelations(addresses));
        relations.append(Object::CreateRelations(objects));
        relations.append(Profile::CreateRelations(profiles));

        return relations;
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

    // Initialize connection
    Q1Connection* conn = new Q1Connection(
        Q1Driver::POSTGRE_SQL,
        "localhost",
        "solo1",
        "postgres",
        "123"
        );

    if (!conn->RootConnect() || !conn->Connect())
    {
        qDebug() << "Database connection failed.";
        return 1;
    }

    // Initialize ORM Context
    TestContext ctx(conn);
    ctx.Initialize();  // Creates database/tables/relations if needed

    // Example query
    QList<Person> people = ctx.persons.Select();


    for (const Person &p : people)
        qDebug() << p.id << p.first_name << p.last_name << p.age;

    conn->Disconnect();
    conn->RootDisconnect();

    return 0;
}
