#ifndef SQLSERVERTESTS_H
#define SQLSERVERTESTS_H

#include "Q1ORMTests.h"

class SqlServerTests : public Q1ORMTests
{
    Q_OBJECT

protected:
    Q1Driver TestDriver() const override;
    QString DriverLabel() const override;
    QString Host() const override;
    QString DatabaseName() const override;
    QString Username() const override;
    QString Password() const override;
    int Port() const override;
};

#endif // SQLSERVERTESTS_H
