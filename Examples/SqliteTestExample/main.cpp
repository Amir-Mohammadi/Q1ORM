#include <QCoreApplication>
#include <QDebug>
#include <QByteArray>

#include "applicationdbcontext.h"


// ============================================================
// MAIN
// ============================================================

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    // ========================================================
    // DATABASE CONNECTION (SQLite)
    // ========================================================



    const QString dbPath = QCoreApplication::applicationDirPath() + "/sqlite3_test.db";

    auto conn = new Q1Connection(
        Q1Driver::SQLITE,
        QString(),
        dbPath,
        QString(),
        QString(),
        0
        );

    ApplicationDbContext ctx(conn);

    if (!ctx.Initialize())
    {
        qWarning()
            << "SQLite initialization failed:"
            << conn->ErrorMessage();

        return -1;
    }

    qDebug()
        << "Connected and initialized SQLite successfully!\n"
        << "Database file:" << conn->GetDatabaseName();


    // ========================================================
    // TEST 1
    // INSERT INTO 5 RELATED TABLES
    // ========================================================

    qDebug() << "========================================";
    qDebug() << "       TEST 1: INSERT 5 TABLES";
    qDebug() << "========================================\n";


    // --------------------------------------------------------
    // 1. COUNTRY
    // --------------------------------------------------------

    qDebug() << "1. Inserting Country...";

    Country country;

    country.id = 0;
    country.name = "USA";

    if (!ctx.countries.Insert(country))
    {
        qWarning()
            << "Country insert failed:"
            << conn->ErrorMessage();

        return -1;
    }

    qDebug()
        << "Country inserted successfully."
        << "ID =" << country.id;


    // --------------------------------------------------------
    // 2. CITY
    // --------------------------------------------------------

    qDebug() << "\n2. Inserting City...";

    City city;

    city.id = 0;
    city.name = "New York";
    city.country_id = country.id;

    if (!ctx.cities.Insert(city))
    {
        qWarning()
            << "City insert failed:"
            << conn->ErrorMessage();

        return -1;
    }

    qDebug()
        << "City inserted successfully."
        << "ID =" << city.id;


    // --------------------------------------------------------
    // 3. CLUB
    // --------------------------------------------------------

    qDebug() << "\n3. Inserting Club...";

    Club club;

    club.id = 0;
    club.name = "New York Padel Club";
    club.city_id = city.id;

    if (!ctx.clubs.Insert(club))
    {
        qWarning()
            << "Club insert failed:"
            << conn->ErrorMessage();

        return -1;
    }

    qDebug()
        << "Club inserted successfully."
        << "ID =" << club.id;


    // --------------------------------------------------------
    // 4. COURT
    // --------------------------------------------------------

    qDebug() << "\n4. Inserting Court...";

    Court court;

    court.id = 0;
    court.name = "Court 1";
    court.club_id = club.id;

    if (!ctx.courts.Insert(court))
    {
        qWarning()
            << "Court insert failed:"
            << conn->ErrorMessage();

        return -1;
    }

    qDebug()
        << "Court inserted successfully."
        << "ID =" << court.id;


    // --------------------------------------------------------
    // 5. USER
    // --------------------------------------------------------

    qDebug() << "\n5. Inserting User...";

    User user;

    user.id = 0;
    user.UserName = "Amir";
    user.Age = 26;
    user.court_id = court.id;

    if (!ctx.users.Insert(user))
    {
        qWarning()
            << "User insert failed:"
            << conn->ErrorMessage();

        return -1;
    }

    qDebug()
        << "User inserted successfully."
        << "ID =" << user.id;


    // ========================================================
    // INSERT SUMMARY
    // ========================================================

    qDebug() << "\n----------------------------------------";
    qDebug() << "INSERT SUMMARY";
    qDebug() << "----------------------------------------";

    qDebug() << "Country ID :" << country.id;
    qDebug() << "City ID    :" << city.id;
    qDebug() << "Club ID    :" << club.id;
    qDebug() << "Court ID   :" << court.id;
    qDebug() << "User ID    :" << user.id;


    // ========================================================
    // TEST 2
    // SELECT ALL 5 TABLES
    // ========================================================

    qDebug() << "\n\n========================================";
    qDebug() << "       TEST 2: SELECT 5 TABLES";
    qDebug() << "========================================\n";


    qDebug() << "\n--- Countries ---";
    ctx.countries
        .Select()
        .ShowList();


    qDebug() << "\n--- Cities ---";
    ctx.cities
        .Select()
        .ShowList();


    qDebug() << "\n--- Clubs ---";
    ctx.clubs
        .Select()
        .ShowList();


    qDebug() << "\n--- Courts ---";
    ctx.courts
        .Select()
        .ShowList();


    qDebug() << "\n--- Users ---";
    ctx.users
        .Select()
        .ShowList();


    // ========================================================
    // TEST 3
    // 5 TABLE INNER JOIN
    // ========================================================

    qDebug() << "\n\n========================================";
    qDebug() << "       TEST 3: INNER JOIN 5 TABLES";
    qDebug() << "========================================\n";


    ctx.countries.Select({

                     "countries.id AS country_id",
                         "countries.name AS country_name",

                         "cities.id AS city_id",
                         "cities.name AS city_name",

                         "clubs.id AS club_id",
                         "clubs.name AS club_name",

                         "courts.id AS court_id",
                         "courts.name AS court_name",

                         "users.id AS user_id",
                         "users.UserName AS user_name",
                         "users.Age AS user_age"

                 })

        // Country -> City
        .InnerJoin(
            "cities",
            "countries.id = cities.country_id"
            )

        // City -> Club
        .InnerJoin(
            "clubs",
            "cities.id = clubs.city_id"
            )

        // Club -> Court
        .InnerJoin(
            "courts",
            "clubs.id = courts.club_id"
            )

        // Court -> User
        .InnerJoin(
            "users",
            "courts.id = users.court_id"
            )

        .ShowList();


    // ========================================================
    // TEST 4
    // 5 TABLE JOIN + WHERE
    // ========================================================

    qDebug() << "\n\n========================================";
    qDebug() << "   TEST 4: JOIN 5 TABLES + WHERE";
    qDebug() << "========================================\n";


    ctx.countries.Select({

                     "countries.name AS country",
                         "cities.name AS city",
                         "clubs.name AS club",
                         "courts.name AS court",
                         "users.UserName AS user",
                         "users.Age AS age"

                 })

        .InnerJoin(
            "cities",
            "countries.id = cities.country_id"
            )

        .InnerJoin(
            "clubs",
            "cities.id = clubs.city_id"
            )

        .InnerJoin(
            "courts",
            "clubs.id = courts.club_id"
            )

        .InnerJoin(
            "users",
            "courts.id = users.court_id"
            )

        .Where(
            "users.Age >= 18"
            )

        .ShowList();


    // ========================================================
    // TEST 5
    // 5 TABLE JOIN + ORDER BY
    // ========================================================

    qDebug() << "\n\n========================================";
    qDebug() << " TEST 5: JOIN 5 TABLES + ORDER BY";
    qDebug() << "========================================\n";


    ctx.countries.Select({

                     "countries.name AS country",
                         "cities.name AS city",
                         "clubs.name AS club",
                         "courts.name AS court",
                         "users.UserName AS user"

                 })

        .InnerJoin(
            "cities",
            "countries.id = cities.country_id"
            )

        .InnerJoin(
            "clubs",
            "cities.id = clubs.city_id"
            )

        .InnerJoin(
            "courts",
            "clubs.id = courts.club_id"
            )

        .InnerJoin(
            "users",
            "courts.id = users.court_id"
            )

        .OrderByAsc(
            "users.UserName"
            )

        .ShowList();


    // ========================================================
    // TEST 6
    // 5 TABLE JOIN + WHERE + ORDER BY + LIMIT
    // ========================================================

    qDebug() << "\n\n========================================";
    qDebug() << " TEST 6: JOIN + WHERE + ORDER + LIMIT";
    qDebug() << "========================================\n";


    ctx.countries.Select({

                     "countries.name AS country",
                         "cities.name AS city",
                         "clubs.name AS club",
                         "courts.name AS court",
                         "users.UserName AS user",
                         "users.Age AS age"

                 })

        .InnerJoin(
            "cities",
            "countries.id = cities.country_id"
            )

        .InnerJoin(
            "clubs",
            "cities.id = clubs.city_id"
            )

        .InnerJoin(
            "courts",
            "clubs.id = courts.club_id"
            )

        .InnerJoin(
            "users",
            "courts.id = users.court_id"
            )

        .Where(
            "users.Age >= 18"
            )

        .OrderByAsc(
            "users.UserName"
            )

        .Limit(10)

        .ShowList();


    // ========================================================
    // TEST 7
    // 5 TABLE JOIN -> JSON
    // ========================================================

    qDebug() << "\n\n========================================";
    qDebug() << "       TEST 7: 5 TABLE JOIN JSON";
    qDebug() << "========================================\n";


    ctx.countries.Select({

                     "countries.id AS country_id",
                         "countries.name AS country",

                         "cities.id AS city_id",
                         "cities.name AS city",

                         "clubs.id AS club_id",
                         "clubs.name AS club",

                         "courts.id AS court_id",
                         "courts.name AS court",

                         "users.id AS user_id",
                         "users.UserName AS user",
                         "users.Age AS age"

                 })

        .InnerJoin(
            "cities",
            "countries.id = cities.country_id"
            )

        .InnerJoin(
            "clubs",
            "cities.id = clubs.city_id"
            )

        .InnerJoin(
            "courts",
            "clubs.id = courts.club_id"
            )

        .InnerJoin(
            "users",
            "courts.id = users.court_id"
            )

        .ShowJson();


    // ========================================================
    // TEST 8
    // GET JSON AS QByteArray
    // ========================================================

    qDebug() << "\n\n========================================";
    qDebug() << "       TEST 8: ToJson()";
    qDebug() << "========================================\n";


    QByteArray jsonData =
        ctx.countries.Select({

                         "countries.name AS country",
                             "cities.name AS city",
                             "clubs.name AS club",
                             "courts.name AS court",
                             "users.UserName AS user"

                     })

            .InnerJoin(
                "cities",
                "countries.id = cities.country_id"
                )

            .InnerJoin(
                "clubs",
                "cities.id = clubs.city_id"
                )

            .InnerJoin(
                "courts",
                "clubs.id = courts.club_id"
                )

            .InnerJoin(
                "users",
                "courts.id = users.court_id"
                )

            .ToJson();


    qDebug().noquote()
        << "JSON:";

    qDebug().noquote()
        << jsonData;


    // ========================================================
    // TEST 9
    // 5 TABLE JOIN + GROUP BY
    // ========================================================

    qDebug() << "\n\n========================================";
    qDebug() << "       TEST 9: GROUP BY";
    qDebug() << "========================================\n";


    ctx.countries.Select({

                     "countries.name AS country",
                         "cities.name AS city",
                         "clubs.name AS club",
                         "COUNT(users.id) AS user_count"

                 })

        .InnerJoin(
            "cities",
            "countries.id = cities.country_id"
            )

        .InnerJoin(
            "clubs",
            "cities.id = clubs.city_id"
            )

        .InnerJoin(
            "courts",
            "clubs.id = courts.club_id"
            )

        .InnerJoin(
            "users",
            "courts.id = users.court_id"
            )

        .GroupBy(
            "countries.name, cities.name, clubs.name"
            )

        .ShowList();


    // ========================================================
    // TEST 10
    // 5 TABLE JOIN + GROUP BY + HAVING
    // ========================================================

    qDebug() << "\n\n========================================";
    qDebug() << "       TEST 10: GROUP BY + HAVING";
    qDebug() << "========================================\n";


    ctx.countries.Select({

                     "countries.name AS country",
                         "cities.name AS city",
                         "clubs.name AS club",
                         "COUNT(users.id) AS user_count"

                 })

        .InnerJoin(
            "cities",
            "countries.id = cities.country_id"
            )

        .InnerJoin(
            "clubs",
            "cities.id = clubs.city_id"
            )

        .InnerJoin(
            "courts",
            "clubs.id = courts.club_id"
            )

        .InnerJoin(
            "users",
            "courts.id = users.court_id"
            )

        .GroupBy(
            "countries.name, cities.name, clubs.name"
            )

        .Having(
            "COUNT(users.id) > 0"
            )

        .ShowList();


    // ========================================================
    // FINAL
    // ========================================================

    qDebug() << "\n\n========================================";
    qDebug() << "          ALL TESTS COMPLETED";
    qDebug() << "========================================\n";


    // ========================================================
    // DISCONNECT
    // ========================================================

    conn->Disconnect();
    conn->RootDisconnect();

    qDebug()
        << "Disconnected successfully.";

    return 0;
}
