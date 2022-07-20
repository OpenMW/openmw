#ifndef OPENMW_COMPONENTS_SQLITE3_TRANSACTION_H
#define OPENMW_COMPONENTS_SQLITE3_TRANSACTION_H

#include <memory>

struct sqlite3;

namespace Sqlite3
{
    struct Rollback
    {
        void operator()(sqlite3* handle) const;
    };

    enum class TransactionMode
    {
        Default,
        Deferred,
        Immediate,
        Exclusive,
    };

    class Transaction
    {
    public:
        explicit Transaction(sqlite3& db, TransactionMode mode = TransactionMode::Default);

        void commit();

    private:
        std::unique_ptr<sqlite3, Rollback> mDb;
    };
}

#endif
