#ifndef MWLUA_NEARBYBINDINGS_H
#define MWLUA_NEARBYBINDINGS_H

#include <sol/forward.hpp>

#include "context.hpp"

namespace MWLua
{
    sol::table initNearbyPackage(const Context&);
}

#endif // MWLUA_NEARBYBINDINGS_H
