#ifndef MWLUA_COREBINDINGS_H
#define MWLUA_COREBINDINGS_H

#include <sol/forward.hpp>

#include "context.hpp"

namespace MWLua
{
    void addCoreTimeBindings(sol::table& api, const Context& context);

    sol::table initCorePackage(const Context&);
}

#endif // MWLUA_COREBINDINGS_H
