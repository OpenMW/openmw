#ifndef OPENMW_MWMECHANICS_INVENTORY_H
#define OPENMW_MWMECHANICS_INVENTORY_H

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"

#include "../mwworld/esmstore.hpp"

#include <components/esm3/loadcont.hpp>
#include <components/misc/strings/algorithm.hpp>

#include <algorithm>
#include <string_view>

namespace MWMechanics
{
    template <class T>
    void modifyBaseInventory(const ESM::RefId& actorId, const ESM::RefId& itemId, int amount)
    {
        T copy = *MWBase::Environment::get().getWorld()->getStore().get<T>().find(actorId);
        for (auto& it : copy.mInventory.mList)
        {
            if (it.mItem == itemId)
            {
                const int sign = it.mCount < 1 ? -1 : 1;
                it.mCount = sign * std::max(it.mCount * sign + amount, 0);
                MWBase::Environment::get().getWorld()->getStore().overrideRecord(copy);
                return;
            }
        }
        if (amount > 0)
        {
            ESM::ContItem cont;
            cont.mItem = itemId;
            cont.mCount = amount;
            copy.mInventory.mList.push_back(cont);
            MWBase::Environment::get().getWorld()->getStore().overrideRecord(copy);
        }
    }
}

#endif
