#ifndef MWLUA_COREBINDINGS_H
#define MWLUA_COREBINDINGS_H

#include <sol/forward.hpp>

#include "context.hpp"

namespace MWLua
{
    void addCoreTimeBindings(sol::table& api, const Context& context);

    sol::table initCorePackage(const Context&);

    // Returns `openmw.core`, but disables the functionality that shouldn't
    // be availabe in menu scripts (to prevent cheating in mutiplayer via menu console).
    sol::table initCorePackageForMenuScripts(const Context&);
}

#endif // MWLUA_COREBINDINGS_H
