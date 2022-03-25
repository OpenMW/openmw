#ifndef MWLUA_STATS_H
#define MWLUA_STATS_H

#include "context.hpp"

namespace MWLua
{
    void addActorStatsBindings(sol::table& actor, const Context& context);
    void addNpcStatsBindings(sol::table& npc, const Context& context);
}

#endif
