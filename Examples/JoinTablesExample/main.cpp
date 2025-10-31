#include <Q1Core/Q1Context/Q1Context.h>
#include <QCoreApplication>
#include <Q1ORM.h>
#include "Person.h"

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    Q1Connection conn(Q1Driver::POSTGRE_SQL, "localhost", "testDb", "postgres", "123", 5432);

    Person user(&conn);



    /* ############################################################################### */
    /* ******************************* return data *********************************** */
    /* ############################################################################### */

    qDebug() << "\n========== INNER JOIN TABLE ==========";
    user.Select()
        .InnerJoin("addresses", "persons.address_id = addresses.id")
        .ShowList();

    qDebug() << "\n========== LEFT JOIN TABLE ==========";
    user.Select()
        .LeftJoin("addresses", "persons.address_id = addresses.id")
        .ShowList();

    qDebug() << "\n========== WHERE + ORDER BY TABLE ==========";
    user.Select()
        .InnerJoin("addresses", "persons.address_id = addresses.id")
        .Where("persons.age > 25")
        .OrderByAsc("persons.first_name")
        .ShowList();

    // ===== JSON OUTPUT EXAMPLES =====
    qDebug() << "\n========== JSON OUTPUT ==========";
    user.Select()
        .InnerJoin("addresses", "persons.address_id = addresses.id")
        .Where("persons.age > 20")
        .ShowJson();  // <-- Print JSON

    qDebug() << "\n========== JSON + TABLE PRINT ==========";
    user.Select()
        .InnerJoin("addresses", "persons.address_id = addresses.id")
        .ShowJson();  // <-- Print JSON with pretty formatting

    // ===== COMPLEX QUERIES =====
    qDebug() << "\n========== GROUP BY + HAVING TABLE ==========";
    user.SelectJoin(QStringList() << "addresses.address_name")
        .InnerJoin("addresses", "persons.address_id = addresses.id")
        .GroupBy("addresses.address_name")
        .Having("COUNT(*) > 1")
        .ShowList();

    qDebug() << "\n========== COMPLEX QUERY JSON ==========";
    user.SelectJoin(QStringList()
                    << "addresses.id"
                    << "addresses.address_name"
                    << "AVG(persons.age) as avg_age")
        .InnerJoin("addresses", "persons.address_id = addresses.id")
        .Where("persons.age >= 18")
        .GroupBy("addresses.id, addresses.address_name")
        .Having("COUNT(*) >= 1")
        .ShowJson();





    /* ############################################################################### */
    /* ******************************** print data *********************************** */
    /* ############################################################################### */



    // ===== TABLE OUTPUT EXAMPLES =====
    qDebug() << "\n========== INNER JOIN TABLE ==========";
    user.Select()
        .InnerJoin("addresses", "persons.address_id = addresses.id")
        .ShowList();

    qDebug() << "\n========== LEFT JOIN TABLE ==========";
    user.Select()
        .LeftJoin("addresses", "persons.address_id = addresses.id")
        .ShowList();

    qDebug() << "\n========== WHERE + ORDER BY TABLE ==========";
    user.Select()
        .InnerJoin("addresses", "persons.address_id = addresses.id")
        .Where("persons.age > 25")
        .OrderByAsc("persons.first_name")
        .ShowList();

    // ===== JSON OUTPUT EXAMPLES =====
    qDebug() << "\n========== JSON OUTPUT ==========";
    user.Select()
        .InnerJoin("addresses", "persons.address_id = addresses.id")
        .Where("persons.age > 20")
        .ShowJson();  // <-- Print JSON

    qDebug() << "\n========== JSON + TABLE PRINT ==========";
    user.Select()
        .InnerJoin("addresses", "persons.address_id = addresses.id")
        .ShowJson();  // <-- Print JSON with pretty formatting

    // ===== COMPLEX QUERIES =====
    qDebug() << "\n========== GROUP BY + HAVING TABLE ==========";
    user.SelectJoin(QStringList() << "addresses.address_name")
        .InnerJoin("addresses", "persons.address_id = addresses.id")
        .GroupBy("addresses.address_name")
        .Having("COUNT(*) > 1")
        .ShowList();

    qDebug() << "\n========== COMPLEX QUERY JSON ==========";
    user.SelectJoin(QStringList()
                    << "addresses.id"
                    << "addresses.address_name"
                    << "AVG(persons.age) as avg_age")
        .InnerJoin("addresses", "persons.address_id = addresses.id")
        .Where("persons.age >= 18")
        .GroupBy("addresses.id, addresses.address_name")
        .Having("COUNT(*) >= 1")
        .ShowJson();

    conn.Disconnect();
    return a.exec();
}
