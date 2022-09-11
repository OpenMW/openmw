#ifndef COMPONENTS_MISC_STRINGS_CONVERSION_H
#define COMPONENTS_MISC_STRINGS_CONVERSION_H

#include <string>

namespace Misc::StringUtils
{
    inline const char* u8StringToString(const char8_t* str)
    {
        return reinterpret_cast<const char*>(str);
    }

    inline char* u8StringToString(char8_t* str)
    {
        return reinterpret_cast<char*>(str);
    }

    inline std::string u8StringToString(std::u8string_view str)
    {
        return {str.begin(), str.end()};
    }

    inline std::string u8StringToString(std::u8string&& str)
    {
        return {str.begin(), str.end()};
    }

    inline const char8_t* stringToU8String(const char* str)
    {
        return reinterpret_cast<const char8_t*>(str);  // Undefined behavior if the contents of "char" aren't UTF8 or ASCII.
    }

    inline char8_t* stringToU8String(char* str)
    {
        return reinterpret_cast<char8_t*>(str); // Undefined behavior if the contents of "char" aren't UTF8 or ASCII.
    }

    inline std::u8string stringToU8String(std::string_view str)
    {
        return { str.begin(), str.end() }; // Undefined behavior if the contents of "char" aren't UTF8 or ASCII.
    }

    inline std::u8string stringToU8String(std::string&& str)
    {
        return { str.begin(), str.end() }; // Undefined behavior if the contents of "char" aren't UTF8 or ASCII.
    }
}

#endif //COMPONENTS_MISC_STRINGS_CONVERSION_H