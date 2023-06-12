#include <components/debug/debuglog.hpp>
#include <components/esm3/loadlevlist.hpp>

#include "../mwworld/class.hpp"
#include "../mwworld/esmstore.hpp"
#include "../mwworld/manualref.hpp"
#include "../mwworld/ptr.hpp"

#include "../mwbase/environment.hpp"

#include "actorutil.hpp"
#include "creaturestats.hpp"
#include "levelledlist.hpp"

namespace MWMechanics
{

    ESM::RefId getLevelledItem(
        const ESM::LevelledListBase* levItem, bool creature, Misc::Rng::Generator& prng, std::optional<int> level)
    {
        const std::vector<ESM::LevelledListBase::LevelItem>& items = levItem->mList;

        int playerLevel;
        if (level.has_value())
            playerLevel = *level;
        else
        {
            const MWWorld::Ptr& player = getPlayer();
            playerLevel = player.getClass().getCreatureStats(player).getLevel();
            level = playerLevel;
        }

        if (Misc::Rng::roll0to99(prng) < levItem->mChanceNone)
            return ESM::RefId();

        std::vector<const ESM::RefId*> candidates;
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

        for (const auto& levelledItem : items)
        {
            if (playerLevel >= levelledItem.mLevel && (allLevels || levelledItem.mLevel == highestLevel))
                candidates.push_back(&levelledItem.mId);
        }
        if (candidates.empty())
            return ESM::RefId();
        const ESM::RefId& item = *candidates[Misc::Rng::rollDice(candidates.size(), prng)];

        // Vanilla doesn't fail on nonexistent items in levelled lists
        if (!MWBase::Environment::get().getESMStore()->find(item))
        {
            Log(Debug::Warning) << "Warning: ignoring nonexistent item " << item << " in levelled list "
                                << levItem->mId;
            return ESM::RefId();
        }

        // Is this another levelled item or a real item?
        MWWorld::ManualRef ref(*MWBase::Environment::get().getESMStore(), item, 1);
        if (ref.getPtr().getType() != ESM::ItemLevList::sRecordId
            && ref.getPtr().getType() != ESM::CreatureLevList::sRecordId)
        {
            return item;
        }
        else
        {
            if (ref.getPtr().getType() == ESM::ItemLevList::sRecordId)
                return getLevelledItem(ref.getPtr().get<ESM::ItemLevList>()->mBase, false, prng, level);
            else
                return getLevelledItem(ref.getPtr().get<ESM::CreatureLevList>()->mBase, true, prng, level);
        }
    }
}
