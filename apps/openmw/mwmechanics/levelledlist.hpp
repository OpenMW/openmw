#ifndef OPENMW_MECHANICS_LEVELLEDLIST_H
#define OPENMW_MECHANICS_LEVELLEDLIST_H

#include "../mwworld/ptr.hpp"
#include "../mwworld/manualref.hpp"
#include "../mwworld/class.hpp"
#include "../mwbase/world.hpp"
#include "../mwbase/environment.hpp"
#include "../mwmechanics/creaturestats.hpp"

namespace MWMechanics
{

    /// @return ID of resulting item, or empty if none
    inline std::string getLevelledItem (const ESM::LeveledListBase* levItem, bool creature, unsigned char failChance=0)
    {
        const std::vector<ESM::LeveledListBase::LevelItem>& items = levItem->mList;

        const MWWorld::Ptr& player = MWBase::Environment::get().getWorld()->getPlayerPtr();
        int playerLevel = player.getClass().getCreatureStats(player).getLevel();

        failChance += levItem->mChanceNone;

        int random = std::rand()/ (static_cast<double> (RAND_MAX) + 1) * 100; // [0, 99]
        if (random < failChance)
            return std::string();

        std::vector<std::string> candidates;
        int highestLevel = 0;
        for (std::vector<ESM::LeveledListBase::LevelItem>::const_iterator it = items.begin(); it != items.end(); ++it)
        {
            if (it->mLevel > highestLevel && it->mLevel <= playerLevel)
                highestLevel = it->mLevel;
        }

        // For levelled creatures, the flags are swapped. This file format just makes so much sense.
        bool allLevels = levItem->mFlags & ESM::ItemLevList::AllLevels;
        if (creature)
            allLevels = levItem->mFlags & ESM::CreatureLevList::AllLevels;

        std::pair<int, std::string> highest = std::make_pair(-1, "");
        for (std::vector<ESM::LeveledListBase::LevelItem>::const_iterator it = items.begin(); it != items.end(); ++it)
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
        std::string item = candidates[std::rand()%candidates.size()];

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
                return getLevelledItem(ref.getPtr().get<ESM::ItemLevList>()->mBase, failChance);
            else
                return getLevelledItem(ref.getPtr().get<ESM::CreatureLevList>()->mBase, failChance);
        }
    }

}

#endif
