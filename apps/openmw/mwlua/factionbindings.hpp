#ifndef MWLUA_FACTIONBINDINGS_H
#define MWLUA_FACTIONBINDINGS_H

#include <sol/forward.hpp>

namespace ESM
{
    struct Faction;
}

namespace MWLua
{
    struct Context;

    sol::table initCoreFactionBindings(const Context& context);
    void addMutableFactionType(sol::state_view& lua);
    ESM::Faction tableToFaction(const sol::table& rec);
}

#endif // MWLUA_FACTIONBINDINGS_H
