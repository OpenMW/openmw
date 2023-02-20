#ifndef OPENMW_MECHANICS_LEVELLEDLIST_H
#define OPENMW_MECHANICS_LEVELLEDLIST_H

#include <components/misc/rng.hpp>

namespace ESM
{
    struct LevelledListBase;
    class RefId;
}

namespace MWMechanics
{

    /// @return ID of resulting item, or empty if none
    ESM::RefId getLevelledItem(const ESM::LevelledListBase* levItem, bool creature, Misc::Rng::Generator& prng);

}

#endif
