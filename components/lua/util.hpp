#ifndef COMPONENTS_LUA_UTIL_H
#define COMPONENTS_LUA_UTIL_H

#include <cstdint>
#include <string>

#include <sol/sol.hpp>

#include <components/esm/refid.hpp>

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

// ADL-based customization point for sol2 to automatically convert ESM::RefId
// Empty RefIds are converted to nil, non-empty ones are serialized to strings
inline int sol_lua_push(sol::types<ESM::RefId>, lua_State* L, const ESM::RefId& id)
{
    if (id.empty())
        return sol::stack::push(L, sol::lua_nil);
    return sol::stack::push(L, id.serializeText());
}

#endif