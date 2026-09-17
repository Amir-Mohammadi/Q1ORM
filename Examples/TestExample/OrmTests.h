#pragma once

#include <QObject>
#include <QTemporaryDir>
#include <memory>
#include "ApplicationContext.h"

class OrmTests : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void init();
    void cleanup();
    void crud();
    void bulkCrud();
    void selects();
    void boundValues();
    void aggregates();
    void grouping();
    void joinsAndIncludes();
    void jsonAndRawSql();
    void changeTracking();
    void transactions();
    void constraints();
    void errorHandling();
    void sqlGeneration();
    void connectionLifecycle();
    void threadedCrud();
    void services();
    void modelBuilder();
    void schemaScale();

private:
    std::unique_ptr<Q1Connection> makeConnection() const;
    QTemporaryDir directory;
    Q1Driver driver = Q1Driver::SQLITE;
    QString database;
    std::unique_ptr<Q1Connection> connection;
    std::unique_ptr<ApplicationContext> context;
    Country firstCountry;
    Country secondCountry;
};
