// Q1ORMTests.h
#ifndef Q1ORMTESTS_H
#define Q1ORMTESTS_H

#include <Q1Core/Q1Context/Q1Connection.h>
#include <QObject>
#include <QtTest/QtTest>
#include "SoloExample/applicationdbcontext.h"
#include "Q1ORM.h"

class Q1ORMTests : public QObject
{
    Q_OBJECT

public:
    explicit Q1ORMTests(Q1Driver driver,
                        QString host,
                        QString databaseName,
                        QString username,
                        QString password,
                        int port,
                        QObject* parent = nullptr);

protected:
    Q1Connection* conn = nullptr;
    ApplicationDbContext* ctx = nullptr;
    Q1Driver configuredDriver;
    QString configuredHost;
    QString configuredDatabaseName;
    QString configuredUsername;
    QString configuredPassword;
    int configuredPort = 0;

    void setupTestData();

    virtual Q1Driver TestDriver() const;
    virtual QString DriverLabel() const;
    virtual QString Host() const;
    virtual QString DatabaseName() const;
    virtual QString Username() const;
    virtual QString Password() const;
    virtual int Port() const;

private slots:
    // Setup and cleanup
    virtual void initTestCase();
    virtual void cleanupTestCase();
    void init();
    void cleanup();

    // Test 1: Basic Select Operations
    void test_selectAll();
    void test_selectSpecificColumns();
    void test_selectAllCountries();

    // Test 2: Where Clause
    void test_whereClause_simple();
    void test_whereClause_comparison();

    // Test 3: Order By
    void test_orderByAsc();
    void test_orderByDesc();
    void test_customOrderBy();

    // Test 4: Limit
    void test_limit();
    void test_limitWithOrderBy();
    void test_skipTakePagination();
    void test_transactionHelpers();
    void test_migrationHistoryTable();
    void test_bulkInsertUpdateDelete();

    // Test 5: Aggregate Functions
    void test_count();
    void test_countWithWhere();
    void test_max();
    void test_min();
    void test_sum();
    void test_avg();
    void test_distinctCount();

    // Test 6: Inner Join
    void test_innerJoin_basic();
    void test_innerJoin_withAliases();
    void test_innerJoin_withWhere();
    void test_innerJoin_withOrderBy();
    void test_innerJoin_withLimit();

    // Test 7: Left Join
    void test_leftJoin();

    // Test 8: Right Join
    void test_rightJoin();

    // Test 9: Full Join
    void test_fullJoin();

    // Test 10: Group By and Having
    void test_groupBy();
    void test_groupByWithHaving();

    // Test 11: Include (Eager Loading)
    void test_include_showList();
    void test_include_showJson();
    void test_include_withWhere();
    void test_include_reverseCollection();

    // Test 12: Combined Operations
    void test_whereOrderByLimit();
    void test_joinWhereOrderByLimit();

    // Test 13: JSON Output
    void test_toJson();
    void test_showJson();

    // Test 14: ToList
    void test_toList();
    void test_toListWithWhere();

    // Test 15: Edge Cases
    void test_emptyResult();
    void test_nullValues();
    void test_reinitialize_is_clean();
};

#endif // Q1ORMTESTS_H

