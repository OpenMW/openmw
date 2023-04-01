#ifndef MWLUA_MWSCRIPTBINDINGS_H
#define MWLUA_MWSCRIPTBINDINGS_H

#include <sol/forward.hpp>

#include "context.hpp"

namespace MWLua
{

    sol::table initMWScriptBindings(const Context&);

}

#endif // MWLUA_MWSCRIPTBINDINGS_H
