#include "content.hpp"
#include "element.hpp"

namespace LuaUi
{
    sol::protected_function loadContentConstructor(LuaUtil::LuaState* state)
    {
        sol::protected_function loader = state->loadInternalLib("content");
        sol::set_environment(state->newInternalLibEnvironment(), loader);
        sol::table metatable = LuaUtil::LuaState::throwIfError(loader()).get<sol::table>();
        if (metatable["new"].get_type() != sol::type::function)
            throw std::logic_error("Expected function");
        return metatable["new"].get<sol::protected_function>();
    }

    bool isValidContent(const sol::object& object)
    {
        if (object.is<Element>())
            return true;
        if (object.get_type() != sol::type::table)
            return false;
        sol::table table = object;
        return table.traverse_get<sol::optional<bool>>(sol::metatable_key, "__Content").value_or(false);
    }
}
