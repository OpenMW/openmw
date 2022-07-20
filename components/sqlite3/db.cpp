#include "db.hpp"

#include <sqlite3.h>

#include <stdexcept>
#include <string>
#include <string_view>

namespace Sqlite3
{
    void CloseSqlite3::operator()(sqlite3* handle) const noexcept
    {
        sqlite3_close_v2(handle);
    }

    Db makeDb(std::string_view path, const char* schema)
    {
        sqlite3* handle = nullptr;
        // All uses of NavMeshDb are protected by a mutex (navmeshtool) or serialized in a single thread (DbWorker)
        // so additional synchronization between threads is not required and SQLITE_OPEN_NOMUTEX can be used.
        // This is unsafe to use NavMeshDb without external synchronization because of internal state.
        const int flags = SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_NOMUTEX;
        if (const int ec = sqlite3_open_v2(std::string(path).c_str(), &handle, flags, nullptr); ec != SQLITE_OK)
        {
            const std::string message(sqlite3_errmsg(handle));
            sqlite3_close(handle);
            throw std::runtime_error("Failed to open database: " + message);
        }
        Db result(handle);
        if (const int ec = sqlite3_exec(result.get(), schema, nullptr, nullptr, nullptr); ec != SQLITE_OK)
            throw std::runtime_error("Failed create database schema: " + std::string(sqlite3_errmsg(handle)));
        return result;
    }
}
