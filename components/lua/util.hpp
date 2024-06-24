#ifndef COMPONENTS_LUA_UTIL_H
#define COMPONENTS_LUA_UTIL_H

#include <cstdint>

namespace LuaUtil
{
    // Lua arrays index from 1
    constexpr inline std::int64_t fromLuaIndex(std::int64_t i)
    {
        return i - 1;
    }

    constexpr inline std::int64_t toLuaIndex(std::int64_t i)
    {
        return i + 1;
    }
}

#endif
