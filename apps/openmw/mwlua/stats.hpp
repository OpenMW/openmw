#ifndef MWLUA_STATS_H
#define MWLUA_STATS_H

#include <sol/forward.hpp>

namespace MWLua
{
    struct Context;

    void addActorStatsBindings(sol::table& actor, const Context& context);
    void addNpcStatsBindings(sol::table& npc, const Context& context);
    sol::table initCoreStatsBindings(const Context& context);
}

#endif
