#ifndef MWLUA_FACTIONBINDINGS_H
#define MWLUA_FACTIONBINDINGS_H

#include <sol/forward.hpp>

namespace MWLua
{
    struct Context;

    sol::table initCoreFactionBindings(const Context& context);
}

#endif // MWLUA_FACTIONBINDINGS_H
