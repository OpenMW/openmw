#ifndef MWLUA_FACTIONBINDINGS_H
#define MWLUA_FACTIONBINDINGS_H

#include <sol/forward.hpp>

#include "context.hpp"

namespace MWLua
{
    sol::table initCoreFactionBindings(const Context& context);
}

#endif // MWLUA_FACTIONBINDINGS_H
