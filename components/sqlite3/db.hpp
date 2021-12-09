#ifndef OPENMW_COMPONENTS_SQLITE3_DB_H
#define OPENMW_COMPONENTS_SQLITE3_DB_H

#include <memory>
#include <string_view>

struct sqlite3;

namespace Sqlite3
{
    struct CloseSqlite3
    {
        void operator()(sqlite3* handle) const noexcept;
    };

    using Db = std::unique_ptr<sqlite3, CloseSqlite3>;

    Db makeDb(std::string_view path, const char* schema);
}

#endif
