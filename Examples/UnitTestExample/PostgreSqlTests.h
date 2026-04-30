#ifndef POSTGRESQLTESTS_H
#define POSTGRESQLTESTS_H

#include "Q1ORMTests.h"

class PostgreSqlTests : public Q1ORMTests
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

#endif // POSTGRESQLTESTS_H
