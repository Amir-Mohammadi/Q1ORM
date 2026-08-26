#ifndef Q1CONNECTIONPOOL_H
#define Q1CONNECTIONPOOL_H

#include <QList>
#include <QMutex>
#include <QQueue>
#include <QTimer>
#include <QObject>
#include "Q1Connection.h"

// A single pooled connection wrapper.
struct Q1PooledConnection
{
    Q1Connection* connection = nullptr;
    qint64 lastUsed = 0;
    bool inUse = false;
};

// Thread-safe connection pool.
// Usage:
//   Q1ConnectionPool::instance().Initialize(driver, host, db, user, pass, port, poolSize);
//   Q1Connection* conn = Q1ConnectionPool::instance().Acquire();
//   // ... use conn ...
//   Q1ConnectionPool::instance().Release(conn);
//
class Q1ORM_EXPORT Q1ConnectionPool : public QObject
{
    Q_OBJECT

public:
    static Q1ConnectionPool& instance();

    void Initialize(Q1Driver driver,
                    const QString& host,
                    const QString& database,
                    const QString& user,
                    const QString& password,
                    int port = 0,
                    int poolSize = 5,
                    int idleTimeoutMs = 60000);

    Q1Connection* Acquire(int timeoutMs = 30000);
    void Release(Q1Connection* conn);
    void Shutdown();

    int ActiveCount() const;
    int IdleCount() const;
    int PoolSize() const { return m_poolSize; }

signals:
    void poolExhausted(int activeCount);

private slots:
    void RecycleIdleConnections();

private:
    explicit Q1ConnectionPool(QObject* parent = nullptr);
    ~Q1ConnectionPool() override;
    Q1ConnectionPool(const Q1ConnectionPool&) = delete;
    Q1ConnectionPool& operator=(const Q1ConnectionPool&) = delete;

    Q1Connection* CreateConnection();
    void DestroyConnection(Q1Connection* conn);

    mutable QMutex m_mutex;
    QList<Q1PooledConnection> m_pool;
    Q1Driver m_driver;
    QString m_host, m_database, m_user, m_password;
    int m_port = 0, m_poolSize = 5, m_idleTimeoutMs = 60000;
    bool m_initialized = false;
    QTimer* m_recycleTimer = nullptr;
};

#endif // Q1CONNECTIONPOOL_H
