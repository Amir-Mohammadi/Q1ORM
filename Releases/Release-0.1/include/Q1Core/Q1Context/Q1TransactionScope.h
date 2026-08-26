#ifndef Q1TRANSACTIONSCOPE_H
#define Q1TRANSACTIONSCOPE_H

#include "Q1Connection.h"

// RAII transaction scope --- auto-rollback on destruction if not committed.
// Usage:
//   Q1TransactionScope tx(connection);
//   // ... do work ...
//   tx.Commit();  // or let destructor rollback
//
class Q1ORM_EXPORT Q1TransactionScope
{
public:
    explicit Q1TransactionScope(Q1Connection& conn)
        : connection(&conn), committed(false), rolled_back(false)
    {
        if (!connection->BeginTransaction())
        {
            failed = true;
            error = connection->ErrorMessage();
        }
    }

    ~Q1TransactionScope()
    {
        if (!committed && !rolled_back && !failed)
        {
            connection->RollbackTransaction();
        }
    }

    Q1TransactionScope(const Q1TransactionScope&) = delete;
    Q1TransactionScope& operator=(const Q1TransactionScope&) = delete;
    Q1TransactionScope(Q1TransactionScope&&) = delete;
    Q1TransactionScope& operator=(Q1TransactionScope&&) = delete;

    bool Commit()
    {
        if (committed || rolled_back || failed)
            return false;
        committed = connection->CommitTransaction();
        if (!committed)
            error = connection->ErrorMessage();
        return committed;
    }

    bool Rollback()
    {
        if (committed || rolled_back || failed)
            return false;
        rolled_back = true;
        return connection->RollbackTransaction();
    }

    bool IsFailed() const { return failed; }
    QString Error() const { return error; }

private:
    Q1Connection* connection;
    bool committed, rolled_back, failed = false;
    QString error;
};

#endif // Q1TRANSACTIONSCOPE_H
