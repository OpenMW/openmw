#ifndef MWLUA_IDCOLLECTIONBINDINGS_H
#define MWLUA_IDCOLLECTIONBINDINGS_H

#include <functional>

#include <components/esm/refid.hpp>
#include <components/lua/luastate.hpp>

namespace MWLua
{
    template <class C, class P = std::identity>
    sol::table createReadOnlyRefIdTable(lua_State* lua, const C& container, P projection = {})
    {
        sol::table res(lua, sol::create);
        for (const auto& element : container)
        {
            ESM::RefId id = projection(element);
            if (!id.empty())
                res.add(id.serializeText());
        }
        return LuaUtil::makeReadOnly(res);
    }
}

#endif
