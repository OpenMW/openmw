#ifndef MWLUA_STATS_H
#define MWLUA_STATS_H

#include <sol/forward.hpp>

namespace ESM
{
    struct Attribute;
}

namespace MWLua
{
    struct Context;

    void addActorStatsBindings(sol::table& actor, const Context& context);
    void addNpcStatsBindings(sol::table& npc, const Context& context);
    sol::table initCoreStatsBindings(const Context& context);

    ESM::Attribute tableToAttribute(const sol::table& rec);
    void addMutableAttributeType(sol::state_view& lua);
}

#endif
