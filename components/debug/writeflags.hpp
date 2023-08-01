#ifndef OPENMW_COMPONENTS_DEBUG_WRITEFLAGS_H
#define OPENMW_COMPONENTS_DEBUG_WRITEFLAGS_H

#include <iomanip>
#include <ostream>
#include <string_view>

namespace Debug
{
    template <class T>
    struct FlagString
    {
        T mValue;
        std::string_view mString;
    };

    template <class T, class FlagStrings>
    std::ostream& writeFlags(std::ostream& stream, const T& value, const FlagStrings& flagStrings)
    {
        bool first = true;
        for (const auto& v : flagStrings)
        {
            if ((value & v.mValue) == 0)
                continue;
            if (first)
                first = false;
            else
                stream << " | ";
            stream << v.mString;
        }
        if (first)
            stream << "[None]";
        return stream << " (0x" << std::hex << value << std::resetiosflags(std::ios_base::hex) << ')';
    }
}

#endif
