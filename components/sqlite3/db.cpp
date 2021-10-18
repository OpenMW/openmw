#include "db.hpp"

#include <sqlite3.h>

#include <stdexcept>
#include <string>
#include <string_view>

namespace Sqlite3
{
    void CloseSqlite3::operator()(sqlite3* handle) const noexcept
    {
        sqlite3_close(handle);
    }

    Db makeDb(std::string_view path, const char* schema)
    {
        sqlite3* handle = nullptr;
        const int flags = SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE;
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
