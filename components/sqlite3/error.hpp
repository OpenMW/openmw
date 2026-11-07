#ifndef OPENMW_COMPONENTS_SQLITE3_ERROR_H
#define OPENMW_COMPONENTS_SQLITE3_ERROR_H

#include <stdexcept>
#include <string>

namespace Sqlite3
{
    class Error : public std::runtime_error
    {
    public:
        Error(int code, const std::string& message)
            : std::runtime_error(message)
            , mCode(code)
        {
        }

        int getCode() const noexcept { return mCode; }

        int getPrimaryCode() const noexcept { return mCode & 0xff; }

    private:
        int mCode;
    };
}

#endif
