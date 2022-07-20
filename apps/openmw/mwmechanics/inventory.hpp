#ifndef OPENMW_MWMECHANICS_INVENTORY_H
#define OPENMW_MWMECHANICS_INVENTORY_H

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"

#include "../mwworld/esmstore.hpp"

#include <components/misc/stringops.hpp>
#include <components/esm3/loadcont.hpp>

#include <algorithm>
#include <string>

namespace MWMechanics
{
    template <class T>
    void modifyBaseInventory(const std::string& actorId, const std::string& itemId, int amount)
    {
        T copy = *MWBase::Environment::get().getWorld()->getStore().get<T>().find(actorId);
        for (auto& it : copy.mInventory.mList)
        {
            if (Misc::StringUtils::ciEqual(it.mItem, itemId))
            {
                const int sign = it.mCount < 1 ? -1 : 1;
                it.mCount = sign * std::max(it.mCount * sign + amount, 0);
                MWBase::Environment::get().getWorld()->createOverrideRecord(copy);
                return;
            }
        }
        if (amount > 0)
        {
            ESM::ContItem cont;
            cont.mItem = itemId;
            cont.mCount = amount;
            copy.mInventory.mList.push_back(cont);
            MWBase::Environment::get().getWorld()->createOverrideRecord(copy);
        }
    }
}

#endif
