#ifndef MWLUA_TYPES_H
#define MWLUA_TYPES_H

#include <sol/sol.hpp>

#include "../context.hpp"

namespace MWLua
{
    void addDoorBindings(sol::table door, const Context& context);
    void addActorBindings(sol::table actor, const Context& context);
}

#endif // MWLUA_TYPES_H
