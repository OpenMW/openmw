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

    class Transaction
    {
    public:
        Transaction(sqlite3& db);

        void commit();

    private:
        std::unique_ptr<sqlite3, Rollback> mDb;
    };
}

#endif
