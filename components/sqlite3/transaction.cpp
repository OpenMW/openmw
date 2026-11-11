#include "transaction.hpp"
#include "error.hpp"

#include <components/debug/debuglog.hpp>

#include <sqlite3.h>

#include <format>
#include <stdexcept>
#include <string>

namespace Sqlite3
{
    namespace
    {
        const char* getBeginStatement(TransactionMode mode)
        {
            switch (mode)
            {
                case TransactionMode::Default:
                    return "BEGIN";
                case TransactionMode::Deferred:
                    return "BEGIN DEFERRED";
                case TransactionMode::Immediate:
                    return "BEGIN IMMEDIATE";
                case TransactionMode::Exclusive:
                    return "BEGIN EXCLUSIVE";
            }
            throw std::logic_error(std::format(
                "Invalid transaction mode: {}", static_cast<std::underlying_type_t<TransactionMode>>(mode)));
        }
    }

    void Rollback::operator()(sqlite3* db) const
    {
        if (db == nullptr)
            return;
        if (const int ec = sqlite3_exec(db, "ROLLBACK", nullptr, nullptr, nullptr); ec != SQLITE_OK)
            Log(Debug::Debug) << "Failed to rollback SQLite3 transaction: " << sqlite3_errmsg(db) << " (" << ec << ")";
    }

    Transaction::Transaction(sqlite3& db, TransactionMode mode)
        : mDb(&db)
    {
        if (const int ec = sqlite3_exec(&db, getBeginStatement(mode), nullptr, nullptr, nullptr); ec != SQLITE_OK)
        {
            (void)mDb.release();
            throw Error(sqlite3_extended_errcode(&db),
                std::format("Failed to start transaction: {} ({})", sqlite3_errmsg(&db), ec));
        }
    }

    void Transaction::commit()
    {
        if (const int ec = sqlite3_exec(mDb.get(), "COMMIT", nullptr, nullptr, nullptr); ec != SQLITE_OK)
            throw Error(sqlite3_extended_errcode(mDb.get()),
                std::format("Failed to commit transaction: {} ({})", sqlite3_errmsg(mDb.get()), ec));
        (void)mDb.release();
    }
}
