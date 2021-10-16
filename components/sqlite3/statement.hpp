#ifndef OPENMW_COMPONENTS_SQLITE3_STATEMENT_H
#define OPENMW_COMPONENTS_SQLITE3_STATEMENT_H

#include <memory>
#include <string_view>
#include <utility>

struct sqlite3;
struct sqlite3_stmt;

namespace Sqlite3
{
    struct CloseSqlite3Stmt
    {
        void operator()(sqlite3_stmt* handle) const noexcept;
    };

    using StatementHandle = std::unique_ptr<sqlite3_stmt, CloseSqlite3Stmt>;

    StatementHandle makeStatementHandle(sqlite3& db, std::string_view query);

    template <class Query>
    struct Statement
    {
        bool mNeedReset = false;
        StatementHandle mHandle;
        Query mQuery;

        explicit Statement(sqlite3& db, Query query = Query {})
            : mHandle(makeStatementHandle(db, query.text())),
              mQuery(std::move(query)) {}
    };
}

#endif
