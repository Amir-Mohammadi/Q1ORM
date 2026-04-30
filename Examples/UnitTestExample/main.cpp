#include <QCoreApplication>
#include <QtTest/QtTest>

#include "PostgreSqlTests.h"
#include "SqlGenerationTests.h"
#include "SqlServerTests.h"

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    int status = 0;

    {
        SqlGenerationTests tests;
        status |= QTest::qExec(&tests, argc, argv);
    }

    {
        PostgreSqlTests tests;
        status |= QTest::qExec(&tests, argc, argv);
    }

    {
        SqlServerTests tests;
        status |= QTest::qExec(&tests, argc, argv);
    }

    return status;
}
