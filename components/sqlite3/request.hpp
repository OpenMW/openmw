#ifndef OPENMW_COMPONENTS_SQLITE3_REQUEST_H
#define OPENMW_COMPONENTS_SQLITE3_REQUEST_H

#include "statement.hpp"
#include "types.hpp"

#include <sqlite3.h>

#include <concepts>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

namespace Sqlite3
{
    template <class T>
    concept StrongTypedef = requires(T v)
    {
        v.mValue;
    };

    inline void bindParameter(sqlite3& db, sqlite3_stmt& stmt, int index, int value)
    {
        if (const int ec = sqlite3_bind_int(&stmt, index, value); ec != SQLITE_OK)
            throw std::runtime_error(
                "Failed to bind int to parameter " + std::to_string(index) + ": " + std::string(sqlite3_errmsg(&db)));
    }

    inline void bindParameter(sqlite3& db, sqlite3_stmt& stmt, int index, std::int64_t value)
    {
        if (const int ec = sqlite3_bind_int64(&stmt, index, value); ec != SQLITE_OK)
            throw std::runtime_error(
                "Failed to bind int64 to parameter " + std::to_string(index) + ": " + std::string(sqlite3_errmsg(&db)));
    }

    inline void bindParameter(sqlite3& db, sqlite3_stmt& stmt, int index, double value)
    {
        if (const int ec = sqlite3_bind_double(&stmt, index, value); ec != SQLITE_OK)
            throw std::runtime_error("Failed to bind double to parameter " + std::to_string(index) + ": "
                + std::string(sqlite3_errmsg(&db)));
    }

    inline void bindParameter(sqlite3& db, sqlite3_stmt& stmt, int index, std::string_view value)
    {
        if (sqlite3_bind_text(&stmt, index, value.data(), static_cast<int>(value.size()), SQLITE_STATIC) != SQLITE_OK)
            throw std::runtime_error(
                "Failed to bind text to parameter " + std::to_string(index) + ": " + std::string(sqlite3_errmsg(&db)));
    }

    inline void bindParameter(sqlite3& db, sqlite3_stmt& stmt, int index, const std::vector<std::byte>& value)
    {
        if (sqlite3_bind_blob(&stmt, index, value.data(), static_cast<int>(value.size()), SQLITE_STATIC) != SQLITE_OK)
            throw std::runtime_error(
                "Failed to bind blob to parameter " + std::to_string(index) + ": " + std::string(sqlite3_errmsg(&db)));
    }

    inline void bindParameter(sqlite3& db, sqlite3_stmt& stmt, int index, const ConstBlob& value)
    {
        if (sqlite3_bind_blob(&stmt, index, value.mData, value.mSize, SQLITE_STATIC) != SQLITE_OK)
            throw std::runtime_error(
                "Failed to bind blob to parameter " + std::to_string(index) + ": " + std::string(sqlite3_errmsg(&db)));
    }

    template <typename T>
    inline void bindParameter(sqlite3& db, sqlite3_stmt& stmt, const char* name, const T& value)
    {
        const int index = sqlite3_bind_parameter_index(&stmt, name);
        if (index == 0)
            throw std::logic_error("Parameter \"" + std::string(name) + "\" is not found");
        bindParameter(db, stmt, index, value);
    }

    inline std::string sqliteTypeToString(int value)
    {
        switch (value)
        {
            case SQLITE_INTEGER:
                return "SQLITE_INTEGER";
            case SQLITE_FLOAT:
                return "SQLITE_FLOAT";
            case SQLITE_TEXT:
                return "SQLITE_TEXT";
            case SQLITE_BLOB:
                return "SQLITE_BLOB";
            case SQLITE_NULL:
                return "SQLITE_NULL";
        }
        return "unsupported(" + std::to_string(value) + ")";
    }

    template <class T>
    inline auto copyColumn(sqlite3& /*db*/, sqlite3_stmt& /*statement*/, int index, int type, T*& value)
    {
        if (type != SQLITE_NULL)
            throw std::logic_error("Type of column " + std::to_string(index) + " is " + sqliteTypeToString(type)
                + " that does not match expected output type: SQLITE_NULL");
        value = nullptr;
    }

    template <std::integral T>
    inline auto copyColumn(sqlite3& /*db*/, sqlite3_stmt& statement, int index, int type, T& value)
    {
        switch (type)
        {
            case SQLITE_INTEGER:
                value = static_cast<T>(sqlite3_column_int64(&statement, index));
                return;
            case SQLITE_NULL:
                value = std::decay_t<T>{};
                return;
        }
        throw std::logic_error("Type of column " + std::to_string(index) + " is " + sqliteTypeToString(type)
            + " that does not match expected output type: SQLITE_INTEGER or SQLITE_NULL");
    }

    template <std::floating_point T>
    inline auto copyColumn(sqlite3& /*db*/, sqlite3_stmt& statement, int index, int type, T& value)
    {
        switch (type)
        {
            case SQLITE_FLOAT:
                value = static_cast<T>(sqlite3_column_double(&statement, index));
                return;
            case SQLITE_NULL:
                value = std::decay_t<T>{};
                return;
        }
        throw std::logic_error("Type of column " + std::to_string(index) + " is " + sqliteTypeToString(type)
            + " that does not match expected output type: SQLITE_FLOAT or SQLITE_NULL");
    }

    template <StrongTypedef T>
    inline auto copyColumn(sqlite3& db, sqlite3_stmt& statement, int index, int type, T& value)
    {
        return copyColumn(db, statement, index, type, value.mValue);
    }

    inline void copyColumn(sqlite3& db, sqlite3_stmt& statement, int index, int type, std::string& value)
    {
        if (type != SQLITE_TEXT)
            throw std::logic_error("Type of column " + std::to_string(index) + " is " + sqliteTypeToString(type)
                + " that does not match expected output type: SQLITE_TEXT");
        const unsigned char* const text = sqlite3_column_text(&statement, index);
        if (text == nullptr)
        {
            if (const int ec = sqlite3_errcode(&db); ec != SQLITE_OK)
                throw std::runtime_error(
                    "Failed to read text from column " + std::to_string(index) + ": " + sqlite3_errmsg(&db));
            value.clear();
            return;
        }
        const int size = sqlite3_column_bytes(&statement, index);
        if (size <= 0)
        {
            if (const int ec = sqlite3_errcode(&db); ec != SQLITE_OK)
                throw std::runtime_error(
                    "Failed to get column bytes " + std::to_string(index) + ": " + sqlite3_errmsg(&db));
            value.clear();
            return;
        }
        value.reserve(static_cast<std::size_t>(size));
        value.assign(reinterpret_cast<const char*>(text), reinterpret_cast<const char*>(text) + size);
    }

    inline void copyColumn(sqlite3& db, sqlite3_stmt& statement, int index, int type, std::vector<std::byte>& value)
    {
        if (type != SQLITE_BLOB)
            throw std::logic_error("Type of column " + std::to_string(index) + " is " + sqliteTypeToString(type)
                + " that does not match expected output type: SQLITE_BLOB");
        const void* const blob = sqlite3_column_blob(&statement, index);
        if (blob == nullptr)
        {
            if (const int ec = sqlite3_errcode(&db); ec != SQLITE_OK)
                throw std::runtime_error(
                    "Failed to read blob from column " + std::to_string(index) + ": " + sqlite3_errmsg(&db));
            value.clear();
            return;
        }
        const int size = sqlite3_column_bytes(&statement, index);
        if (size <= 0)
        {
            if (const int ec = sqlite3_errcode(&db); ec != SQLITE_OK)
                throw std::runtime_error(
                    "Failed to get column bytes " + std::to_string(index) + ": " + sqlite3_errmsg(&db));
            value.clear();
            return;
        }
        value.reserve(static_cast<std::size_t>(size));
        value.assign(static_cast<const std::byte*>(blob), static_cast<const std::byte*>(blob) + size);
    }

    template <int index, class T>
    inline void getColumnsImpl(sqlite3& db, sqlite3_stmt& statement, T& row)
    {
        if constexpr (0 < index && index <= std::tuple_size_v<T>)
        {
            const int column = index - 1;
            if (const int number = sqlite3_column_count(&statement); column >= number)
                throw std::out_of_range(
                    "Column number is out of range: " + std::to_string(column) + " >= " + std::to_string(number));
            const int type = sqlite3_column_type(&statement, column);
            switch (type)
            {
                case SQLITE_INTEGER:
                case SQLITE_FLOAT:
                case SQLITE_TEXT:
                case SQLITE_BLOB:
                case SQLITE_NULL:
                    copyColumn(db, statement, column, type, std::get<index - 1>(row));
                    break;
                default:
                    throw std::runtime_error("Column " + std::to_string(column)
                        + " has unnsupported column type: " + sqliteTypeToString(type));
            }
            getColumnsImpl<index - 1>(db, statement, row);
        }
    }

    template <class T>
    inline void getColumns(sqlite3& db, sqlite3_stmt& statement, T& row)
    {
        getColumnsImpl<std::tuple_size_v<T>>(db, statement, row);
    }

    template <class T>
    inline void getRow(sqlite3& db, sqlite3_stmt& statement, T& row)
    {
        auto tuple = std::tie(row);
        getColumns(db, statement, tuple);
    }

    template <class... Args>
    inline void getRow(sqlite3& db, sqlite3_stmt& statement, std::tuple<Args...>& row)
    {
        getColumns(db, statement, row);
    }

    template <class T>
    inline void getRow(sqlite3& db, sqlite3_stmt& statement, std::back_insert_iterator<T>& it)
    {
        typename T::value_type row;
        getRow(db, statement, row);
        it = std::move(row);
    }

    template <class T, class... Args>
    inline void prepare(sqlite3& db, Statement<T>& statement, Args&&... args)
    {
        if (statement.mNeedReset)
        {
            if (sqlite3_reset(statement.mHandle.get()) == SQLITE_OK
                && sqlite3_clear_bindings(statement.mHandle.get()) == SQLITE_OK)
                statement.mNeedReset = false;
            else
                statement.mHandle = makeStatementHandle(db, statement.mQuery.text());
        }
        statement.mQuery.bind(db, *statement.mHandle, std::forward<Args>(args)...);
    }

    template <class T>
    inline bool executeStep(sqlite3& db, const Statement<T>& statement)
    {
        switch (sqlite3_step(statement.mHandle.get()))
        {
            case SQLITE_ROW:
                return true;
            case SQLITE_DONE:
                return false;
        }
        throw std::runtime_error("Failed to execute statement step: " + std::string(sqlite3_errmsg(&db)));
    }

    template <class T, class I, class... Args>
    inline I request(sqlite3& db, Statement<T>& statement, I out, std::size_t max, Args&&... args)
    {
        try
        {
            statement.mNeedReset = true;
            prepare(db, statement, std::forward<Args>(args)...);
            for (std::size_t i = 0; executeStep(db, statement) && i < max; ++i)
                getRow(db, *statement.mHandle, *out++);
            return out;
        }
        catch (const std::exception& e)
        {
            throw std::runtime_error(
                "Failed perform request \"" + std::string(statement.mQuery.text()) + "\": " + std::string(e.what()));
        }
    }

    template <class T, class... Args>
    inline int execute(sqlite3& db, Statement<T>& statement, Args&&... args)
    {
        try
        {
            statement.mNeedReset = true;
            prepare(db, statement, std::forward<Args>(args)...);
            if (executeStep(db, statement))
                throw std::logic_error("Execute cannot return rows");
            return sqlite3_changes(&db);
        }
        catch (const std::exception& e)
        {
            throw std::runtime_error("Failed to execute statement \"" + std::string(statement.mQuery.text())
                + "\": " + std::string(e.what()));
        }
    }
}

#endif
