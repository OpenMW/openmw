#include "content.hpp"

namespace LuaUi::Content
{
    namespace
    {
        sol::table loadMetatable(sol::state_view sol)
        {
            std::string scriptBody =
#include "content.lua"
                ;
            auto result = sol.safe_script(scriptBody);
            if (result.get_type() != sol::type::table)
                throw std::logic_error("Expected a meta table");
            return result.get<sol::table>();
        }
    }

    sol::protected_function makeFactory(sol::state_view sol)
    {
        sol::table metatable = loadMetatable(sol);
        if (metatable["new"].get_type() != sol::type::function)
            throw std::logic_error("Expected function");
        return metatable["new"].get<sol::protected_function>();
    }

    int64_t View::sInstanceCount = 0;

    int64_t getInstanceCount()
    {
        return View::sInstanceCount;
    }
}
