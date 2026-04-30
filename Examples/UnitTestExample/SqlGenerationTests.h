#ifndef SQLGENERATIONTESTS_H
#define SQLGENERATIONTESTS_H

#include <QObject>

class SqlGenerationTests : public QObject
{
    Q_OBJECT

private slots:
    void test_postgresqlTranslatorStillUsesPostgresDialect();
    void test_sqlServerTranslatorBuildsIdentityTable();
    void test_sqlServerTranslatorBuildsDefaultConstraintStatements();
    void test_sqlServerTranslatorUsesMetadataForNullabilityChanges();
};

#endif // SQLGENERATIONTESTS_H
