#ifndef MWLUA_COREBINDINGS_H
#define MWLUA_COREBINDINGS_H

#include <sol/forward.hpp>

namespace MWLua
{
    struct Context;

    void addCoreTimeBindings(sol::table& api, const Context& context);

    sol::table initCorePackage(const Context& context);
}

#endif // MWLUA_COREBINDINGS_H
