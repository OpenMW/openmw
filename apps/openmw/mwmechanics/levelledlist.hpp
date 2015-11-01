#ifndef OPENMW_MECHANICS_LEVELLEDLIST_H
#define OPENMW_MECHANICS_LEVELLEDLIST_H

#include <algorithm>
#include <string>
#include <vector>
#include <iostream>

#include <components/misc/rng.hpp>

#include "../mwworld/ptr.hpp"
#include "../mwworld/esmstore.hpp"
#include "../mwworld/manualref.hpp"
#include "../mwworld/class.hpp"
#include "../mwbase/world.hpp"
#include "../mwbase/environment.hpp"
#include "creaturestats.hpp"
#include "actorutil.hpp"

namespace MWMechanics
{
    /// @return ID of resulting item, or empty if none
    inline std::string getLevelledItem (const ESM::LevelledListBase* levItem, bool creature, unsigned char failChance=0)
    {
        const std::vector<ESM::LevelledListBase::LevelItem>& items = levItem->mList;

        const MWWorld::Ptr& player = getPlayer();
        int playerLevel = player.getClass().getCreatureStats(player).getLevel();

        failChance += levItem->mChanceNone;

        if (Misc::Rng::roll0to99() < failChance)
            return std::string();

        std::vector<std::string> candidates;
        int highestLevel = 0;
        for (std::vector<ESM::LevelledListBase::LevelItem>::const_iterator it = items.begin(); it != items.end(); ++it)
        {
            if (it->mLevel > highestLevel && it->mLevel <= playerLevel)
                highestLevel = it->mLevel;
        }

        // For levelled creatures, the flags are swapped. This file format just makes so much sense.
        bool allLevels = (levItem->mFlags & ESM::ItemLevList::AllLevels) != 0;
        if (creature)
            allLevels = levItem->mFlags & ESM::CreatureLevList::AllLevels;

        std::pair<int, std::string> highest = std::make_pair(-1, "");
        for (std::vector<ESM::LevelledListBase::LevelItem>::const_iterator it = items.begin(); it != items.end(); ++it)
        {
            if (playerLevel >= it->mLevel
                    && (allLevels || it->mLevel == highestLevel))
            {
                candidates.push_back(it->mId);
                if (it->mLevel >= highest.first)
                    highest = std::make_pair(it->mLevel, it->mId);
            }
        }
        if (candidates.empty())
            return std::string();
        std::string item = candidates[Misc::Rng::rollDice(candidates.size())];

        // Vanilla doesn't fail on nonexistent items in levelled lists
        if (!MWBase::Environment::get().getWorld()->getStore().find(Misc::StringUtils::lowerCase(item)))
        {
            std::cerr << "Warning: ignoring nonexistent item '" << item << "' in levelled list '" << levItem->mId << "'" << std::endl;
            return std::string();
        }

        // Is this another levelled item or a real item?
        MWWorld::ManualRef ref (MWBase::Environment::get().getWorld()->getStore(), item, 1);
        if (ref.getPtr().getTypeName() != typeid(ESM::ItemLevList).name()
                && ref.getPtr().getTypeName() != typeid(ESM::CreatureLevList).name())
        {
            return item;
        }
        else
        {
            if (ref.getPtr().getTypeName() == typeid(ESM::ItemLevList).name())
                return getLevelledItem(ref.getPtr().get<ESM::ItemLevList>()->mBase, false, failChance);
            else
                return getLevelledItem(ref.getPtr().get<ESM::CreatureLevList>()->mBase, true, failChance);
        }
    }
}

//Returns list of items in group. You get them by returned value->mId.
inline const std::vector<ESM::LevelledListBase::LevelItem>& itemsInGroup(const std::string& group)
{
    MWWorld::ManualRef ref (MWBase::Environment::get().getWorld()->getStore(), group);
    const ESM::ItemLevList* levItem = ref.getPtr().get<ESM::ItemLevList>()->mBase;
    const std::vector<ESM::LevelledListBase::LevelItem>& items = levItem->mList;
    return items;
}

//Returns true if item @name is in levelled group @group, false if not.
inline bool itemInLevelledGroup(const std::string& name, const std::string& group)
{
    //Get all items from the group
    const std::vector<ESM::LevelledListBase::LevelItem>& elements = itemsInGroup(group);
    //And see if item we're looking for is there.
    for(std::vector<ESM::LevelledListBase::LevelItem>::const_iterator it = elements.begin(); it != elements.end(); ++it)
    {
        if(it->mId == name)
            return true;
    }
    return false;
}

//Return the name of levelled group to which @item belongs, or "" if there's no such group.
//Function searches for item in all groups of @origin, obtained from NPC's store.
inline std::string levelledItemGroup(const std::string& item, const ESM::InventoryList& origin)
{
    //For every item NPC could have
    for(std::vector<ESM::ContItem>::const_iterator it = origin.mList.begin(); it != origin.mList.end(); ++it)
    {
        //Look only for restocking(negative count) items
        if (it->mCount >= 0)
            continue;
        // If the item belongs to levelled list, return it
        if (MWBase::Environment::get().getWorld()->getStore().get<ESM::ItemLevList>().search(it->mItem.toString()))
        {
            std::string group = Misc::StringUtils::lowerCase(it->mItem.toString());
            if(itemInLevelledGroup(item, group))
                return group;
        }
    }
    return std::string();
}

#endif
