#include "content.hpp"

namespace LuaUi::Content
{
    sol::protected_function loadConstructor(LuaUtil::LuaState* state)
    {
        sol::function loader = state->loadInternalLib("content");
        sol::set_environment(state->newInternalLibEnvironment(), loader);
        sol::table metatable = loader().get<sol::table>();
        if (metatable["new"].get_type() != sol::type::function)
            throw std::logic_error("Expected function");
        return metatable["new"].get<sol::protected_function>();
    }
}
