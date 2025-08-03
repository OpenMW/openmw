#ifndef OPENMW_MWLUA_DEBUGBINDINGS_H
#define OPENMW_MWLUA_DEBUGBINDINGS_H

#include <sol/forward.hpp>

namespace MWLua
{
    struct Context;

    sol::table initDebugPackage(const Context& context);
}

#endif // OPENMW_MWLUA_DEBUGBINDINGS_H
