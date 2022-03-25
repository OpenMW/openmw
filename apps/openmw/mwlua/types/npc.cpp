#include "types.hpp"

#include "../stats.hpp"

namespace MWLua
{
    void addNpcBindings(sol::table npc, const Context& context)
    {
        addNpcStatsBindings(npc, context);
    }
}
