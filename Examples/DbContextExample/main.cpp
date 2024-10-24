#include <QCoreApplication>

#include <QString>
#include <QObject>
#include <QSqlDatabase>
#include <QtSql>

#include <Q1Core/Q1Context/Q1Context.h>
#include <Q1Core/Q1Entity/Q1Entity.h>
#include "Address.h"
#include "Person.h"
#include "Object.h"

class TestContext : public Q1Context
{
public:
    TestContext(Q1Connection *connectionPtr) : persons(connectionPtr), objects(connectionPtr), addresses(connectionPtr) {}



    virtual void OnConfiguration()
    {
        connection = new Q1Connection(Q1Driver::POSTGRE_SQL, "localhost", "MyDb", "admin", "123");
    }

    virtual QList<Q1Table> OnTablesCreating()
    {
        ColumnsMigration(true);
        InitTables();

        QList<Q1Table> tables =
        {
            persons.GetTable(),
            objects.GetTable(),
            addresses.GetTable()
        };

        return tables;
    }

    void InitTables()
    {
        // Persons
        // persons.ToTableName("Persons");

        // persons.Property(persons.id, "id", false, true);
        // persons.Property(persons.first_name, "first_name", true);
        // persons.Property(persons.last_name, "last_name", true);
        // persons.Property(persons.age, "age", false, false);
        // persons.Property(persons.address_id,"address_id", true);

        // /// Objects
        // objects.ToTableName("Objects");

        // objects.Property(objects.id, "id", false, true, nullptr);
        // objects.Property(objects.person_id, "person_id", true);
        // objects.Property(objects.name, "name", true);
        // objects.Property(objects.width, "width", true);
        // objects.Property(objects.height, "height", true);
        // objects.Property(objects.weight, "weight", false, false, "60");

        addresses.ToTableName("Addresses");

        addresses.Property(addresses.id, "id", false, true, nullptr);
        addresses.Property(addresses.address_name,"address_name", false ,"default");
        addresses.Property(addresses.person_id, "person_id", true);


    //     QString a = "Objects";
    //     QString b = "Persons";
    //     QString c = "person_id";

    //     objects.Relations(a, b, Q1RelationType::ONE_TO_MANY, c, "id");



    //     /// Addresses



    //     addresses.Relations("Addresses","Persons", Q1RelationType::ONE_TO_ONE, "person_id", "address_id");




    }

public:
    Q1Entity<Person> persons;
    Q1Entity<Object> objects;
    Q1Entity<Address> addresses;
};

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    // Load the PostgreSQL driver
    QSqlDatabase db = QSqlDatabase::addDatabase("QPSQL");

    // Set database connection parameters
    db.setHostName("localhost");
    db.setDatabaseName("testDDvb");
    db.setUserName("admin");
    db.setPassword("123");

    // Check if the database is open
    if (!db.open()) {
        qDebug() << "Error opening database:" << db.lastError().text();
        return 1;
    }

    Q1Connection *connection = new Q1Connection(Q1Driver::POSTGRE_SQL, "localhost", "testDDvb", "admin", "123");
    TestContext context(connection);
    context.Initialize();
    return 0;
}
