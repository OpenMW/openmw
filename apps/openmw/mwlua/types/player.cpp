#include "types.hpp"

#include <apps/openmw/mwmechanics/npcstats.hpp>
#include <apps/openmw/mwworld/class.hpp>

namespace MWLua
{

    void addPlayerBindings(sol::table player, const Context& context)
    {
        player["getCrimeLevel"] = [](const Object& o) -> int {
            const MWWorld::Class& cls = o.ptr().getClass();
            return cls.getNpcStats(o.ptr()).getBounty();
        };
    }
}
