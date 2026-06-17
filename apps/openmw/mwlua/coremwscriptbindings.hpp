#ifndef MWLUA_COREMWSCRIPTBINDINGS_H
#define MWLUA_COREMWSCRIPTBINDINGS_H

#include <sol/forward.hpp>

namespace MWLua
{
    struct Context;

    sol::table initCoreMwScriptBindings(const Context& context);
}

#endif // MWLUA_COREMWSCRIPTBINDINGS_H
