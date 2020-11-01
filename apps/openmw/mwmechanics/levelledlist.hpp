#ifndef OPENMW_MECHANICS_LEVELLEDLIST_H
#define OPENMW_MECHANICS_LEVELLEDLIST_H

#include <components/debug/debuglog.hpp>
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
    inline std::string getLevelledItem (const ESM::LevelledListBase* levItem, bool creature, Misc::Rng::Seed& seed = Misc::Rng::getSeed())
    {
        const std::vector<ESM::LevelledListBase::LevelItem>& items = levItem->mList;

        const MWWorld::Ptr& player = getPlayer();
        int playerLevel = player.getClass().getCreatureStats(player).getLevel();

        if (Misc::Rng::roll0to99(seed) < levItem->mChanceNone)
            return std::string();

        std::vector<std::string> candidates;
        int highestLevel = 0;
        for (const auto& levelledItem : items)
        {
            if (levelledItem.mLevel > highestLevel && levelledItem.mLevel <= playerLevel)
                highestLevel = levelledItem.mLevel;
        }

        // For levelled creatures, the flags are swapped. This file format just makes so much sense.
        bool allLevels = (levItem->mFlags & ESM::ItemLevList::AllLevels) != 0;
        if (creature)
            allLevels = levItem->mFlags & ESM::CreatureLevList::AllLevels;

        std::pair<int, std::string> highest = std::make_pair(-1, "");
        for (const auto& levelledItem : items)
        {
            if (playerLevel >= levelledItem.mLevel
                    && (allLevels || levelledItem.mLevel == highestLevel))
            {
                candidates.push_back(levelledItem.mId);
                if (levelledItem.mLevel >= highest.first)
                    highest = std::make_pair(levelledItem.mLevel, levelledItem.mId);
            }
        }
        if (candidates.empty())
            return std::string();
        std::string item = candidates[Misc::Rng::rollDice(candidates.size(), seed)];

        // Vanilla doesn't fail on nonexistent items in levelled lists
        if (!MWBase::Environment::get().getWorld()->getStore().find(Misc::StringUtils::lowerCase(item)))
        {
            Log(Debug::Warning) << "Warning: ignoring nonexistent item '" << item << "' in levelled list '" << levItem->mId << "'";
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
                return getLevelledItem(ref.getPtr().get<ESM::ItemLevList>()->mBase, false, seed);
            else
                return getLevelledItem(ref.getPtr().get<ESM::CreatureLevList>()->mBase, true, seed);
        }
    }

}

#endif
