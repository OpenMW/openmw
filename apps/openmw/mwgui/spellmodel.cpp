#include "spellmodel.hpp"

#include <boost/lexical_cast.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"
#include "../mwbase/windowmanager.hpp"

#include "../mwmechanics/creaturestats.hpp"
#include "../mwmechanics/spellcasting.hpp"

#include "../mwworld/esmstore.hpp"
#include "../mwworld/inventorystore.hpp"
#include "../mwworld/class.hpp"

namespace
{

    bool sortSpells(const MWGui::Spell& left, const MWGui::Spell& right)
    {
        if (left.mType != right.mType)
            return left.mType < right.mType;

        std::string leftName = Misc::StringUtils::lowerCase(left.mName);
        std::string rightName = Misc::StringUtils::lowerCase(right.mName);

        return leftName.compare(rightName) < 0;
    }

}

namespace MWGui
{

    SpellModel::SpellModel(const MWWorld::Ptr &actor)
        : mActor(actor)
    {

    }

    void SpellModel::update()
    {
        mSpells.clear();

        MWMechanics::CreatureStats& stats = mActor.getClass().getCreatureStats(mActor);
        const MWMechanics::Spells& spells = stats.getSpells();

        const MWWorld::ESMStore &esmStore =
            MWBase::Environment::get().getWorld()->getStore();

        for (MWMechanics::Spells::TIterator it = spells.begin(); it != spells.end(); ++it)
        {
            const ESM::Spell* spell = esmStore.get<ESM::Spell>().find(it->first);
            if (spell->mData.mType != ESM::Spell::ST_Power && spell->mData.mType != ESM::Spell::ST_Spell)
                continue;

            Spell newSpell;
            newSpell.mName = spell->mName;
            if (spell->mData.mType == ESM::Spell::ST_Spell)
            {
                newSpell.mType = Spell::Type_Spell;
                std::string cost = boost::lexical_cast<std::string>(spell->mData.mCost);
                std::string chance = boost::lexical_cast<std::string>(int(MWMechanics::getSpellSuccessChance(spell, mActor)));
                newSpell.mCostColumn = cost + "/" + chance;
            }
            else
                newSpell.mType = Spell::Type_Power;
            newSpell.mId = it->first;

            newSpell.mSelected = (MWBase::Environment::get().getWindowManager()->getSelectedSpell() == it->first);
            newSpell.mActive = true;
            mSpells.push_back(newSpell);
        }

        MWWorld::InventoryStore& invStore = mActor.getClass().getInventoryStore(mActor);
        for (MWWorld::ContainerStoreIterator it = invStore.begin(); it != invStore.end(); ++it)
        {
            MWWorld::Ptr item = *it;
            const std::string enchantId = item.getClass().getEnchantment(item);
            if (enchantId.empty())
                continue;
            const ESM::Enchantment* enchant =
                esmStore.get<ESM::Enchantment>().find(item.getClass().getEnchantment(item));
            if (enchant->mData.mType != ESM::Enchantment::WhenUsed && enchant->mData.mType != ESM::Enchantment::CastOnce)
                continue;

            Spell newSpell;
            newSpell.mItem = item;
            newSpell.mId = item.getClass().getId(item);
            newSpell.mName = item.getClass().getName(item);
            newSpell.mType = Spell::Type_EnchantedItem;
            newSpell.mSelected = invStore.getSelectedEnchantItem() == it;

            // FIXME: move to mwmechanics
            if (enchant->mData.mType == ESM::Enchantment::CastOnce)
            {
                newSpell.mCostColumn = "100/100";
                newSpell.mActive = false;
            }
            else
            {
                if (!item.getClass().getEquipmentSlots(item).first.empty()
                        && item.getClass().canBeEquipped(item, mActor).first == 0)
                    continue;

                int castCost = MWMechanics::getEffectiveEnchantmentCastCost(static_cast<float>(enchant->mData.mCost), mActor);

                std::string cost = boost::lexical_cast<std::string>(castCost);
                int currentCharge = int(item.getCellRef().getEnchantmentCharge());
                if (currentCharge ==  -1)
                    currentCharge = enchant->mData.mCharge;
                std::string charge = boost::lexical_cast<std::string>(currentCharge);
                newSpell.mCostColumn = cost + "/" + charge;

                newSpell.mActive = invStore.isEquipped(item);
            }
            mSpells.push_back(newSpell);
        }

        std::stable_sort(mSpells.begin(), mSpells.end(), sortSpells);
    }

    size_t SpellModel::getItemCount() const
    {
        return mSpells.size();
    }

    Spell SpellModel::getItem(ModelIndex index) const
    {
        if (index < 0 || index >= int(mSpells.size()))
            throw std::runtime_error("invalid spell index supplied");
        return mSpells[index];
    }

}
