#include "transaction.hpp"

#include <components/debug/debuglog.hpp>

#include <sqlite3.h>

#include <stdexcept>
#include <string>

namespace Sqlite3
{
    void Rollback::operator()(sqlite3* db) const
    {
        if (db == nullptr)
            return;
        if (const int ec = sqlite3_exec(db, "ROLLBACK", nullptr, nullptr, nullptr); ec != SQLITE_OK)
            Log(Debug::Warning) << "Failed to rollback SQLite3 transaction: " << std::string(sqlite3_errmsg(db));
    }

    Transaction::Transaction(sqlite3& db)
        : mDb(&db)
    {
        if (const int ec = sqlite3_exec(mDb.get(), "BEGIN", nullptr, nullptr, nullptr); ec != SQLITE_OK)
            throw std::runtime_error("Failed to start transaction: " + std::string(sqlite3_errmsg(mDb.get())));
    }

    void Transaction::commit()
    {
        if (const int ec = sqlite3_exec(mDb.get(), "COMMIT", nullptr, nullptr, nullptr); ec != SQLITE_OK)
            throw std::runtime_error("Failed to commit transaction: " + std::string(sqlite3_errmsg(mDb.get())));
        (void) mDb.release();
    }
}
