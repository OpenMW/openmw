#ifndef OPENMW_MWWORLD_GLOBALVARIABLENAME_H
#define OPENMW_MWWORLD_GLOBALVARIABLENAME_H

#include <cstddef>
#include <string>
#include <string_view>

namespace MWWorld
{
    class Globals;

    class GlobalVariableName
    {
    public:
        GlobalVariableName(const std::string& value)
            : mValue(value)
        {
        }

        GlobalVariableName(std::string_view value)
            : mValue(value)
        {
        }

        std::string_view getValue() const { return mValue; }

        friend bool operator==(const GlobalVariableName& lhs, const GlobalVariableName& rhs) noexcept
        {
            return lhs.mValue == rhs.mValue;
        }

    private:
        std::string_view mValue;

        explicit constexpr GlobalVariableName(const char* value)
            : mValue(value)
        {
        }

        friend Globals;
    };
}

#endif
