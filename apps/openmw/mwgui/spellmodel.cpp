#include "spellmodel.hpp"

#include <components/debug/debuglog.hpp>
#include <components/misc/utf8stream.hpp>

#include <components/esm3/loadench.hpp>
#include <components/esm3/loadmgef.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/windowmanager.hpp"

#include "../mwmechanics/creaturestats.hpp"
#include "../mwmechanics/spellutil.hpp"

#include "../mwworld/class.hpp"
#include "../mwworld/esmstore.hpp"
#include "../mwworld/inventorystore.hpp"

namespace
{

    bool sortSpells(const MWGui::Spell& left, const MWGui::Spell& right)
    {
        if (left.mType != right.mType)
            return left.mType < right.mType;
        return Misc::StringUtils::ciLess(left.mName, right.mName);
    }

}

namespace MWGui
{

    SpellModel::SpellModel(const MWWorld::Ptr& actor, const std::string& filter)
        : mActor(actor)
        , mFilter(filter)
    {
    }

    SpellModel::SpellModel(const MWWorld::Ptr& actor)
        : mActor(actor)
    {
    }

    bool SpellModel::matchingEffectExists(std::string filter, const ESM::EffectList& effects)
    {
        const MWWorld::ESMStore& store = *MWBase::Environment::get().getESMStore();

        for (const auto& effect : effects.mList)
        {
            short effectId = effect.mData.mEffectID;

            if (effectId != -1)
            {
                const ESM::MagicEffect* magicEffect = store.get<ESM::MagicEffect>().find(effectId);
                const ESM::Attribute* attribute
                    = store.get<ESM::Attribute>().search(ESM::Attribute::indexToRefId(effect.mData.mAttribute));
                const ESM::Skill* skill = store.get<ESM::Skill>().search(ESM::Skill::indexToRefId(effect.mData.mSkill));

                std::string fullEffectName = MWMechanics::getMagicEffectString(*magicEffect, attribute, skill);
                std::string convert = Utf8Stream::lowerCaseUtf8(fullEffectName);
                if (convert.find(filter) != std::string::npos)
                {
                    return true;
                }
            }
        }

        return false;
    }

    void SpellModel::update()
    {
        mSpells.clear();

        MWMechanics::CreatureStats& stats = mActor.getClass().getCreatureStats(mActor);
        const MWMechanics::Spells& spells = stats.getSpells();

        const MWWorld::ESMStore& esmStore = *MWBase::Environment::get().getESMStore();

        std::string filter = Utf8Stream::lowerCaseUtf8(mFilter);

        for (const ESM::Spell* spell : spells)
        {
            if (spell->mData.mType != ESM::Spell::ST_Power && spell->mData.mType != ESM::Spell::ST_Spell)
                continue;

            std::string name = Utf8Stream::lowerCaseUtf8(spell->mName);

            if (name.find(filter) == std::string::npos && !matchingEffectExists(filter, spell->mEffects))
                continue;

            Spell newSpell;
            newSpell.mName = spell->mName;
            if (spell->mData.mType == ESM::Spell::ST_Spell)
            {
                newSpell.mType = Spell::Type_Spell;
                std::string cost = std::to_string(MWMechanics::calcSpellCost(*spell));
                std::string chance = std::to_string(int(MWMechanics::getSpellSuccessChance(spell, mActor)));
                newSpell.mCostColumn = cost + "/" + chance;
            }
            else
                newSpell.mType = Spell::Type_Power;
            newSpell.mId = spell->mId;

            newSpell.mSelected = (MWBase::Environment::get().getWindowManager()->getSelectedSpell() == spell->mId);
            newSpell.mActive = true;
            newSpell.mCount = 1;
            mSpells.push_back(newSpell);
        }

        MWWorld::InventoryStore& invStore = mActor.getClass().getInventoryStore(mActor);
        for (MWWorld::ContainerStoreIterator it = invStore.begin(); it != invStore.end(); ++it)
        {
            MWWorld::Ptr item = *it;
            const ESM::RefId& enchantId = item.getClass().getEnchantment(item);
            if (enchantId.empty())
                continue;
            const ESM::Enchantment* enchant = esmStore.get<ESM::Enchantment>().search(enchantId);
            if (!enchant)
            {
                Log(Debug::Warning) << "Warning: Can't find enchantment '" << enchantId << "' on item "
                                    << item.getCellRef().getRefId();
                continue;
            }

            if (enchant->mData.mType != ESM::Enchantment::WhenUsed
                && enchant->mData.mType != ESM::Enchantment::CastOnce)
                continue;

            std::string name = Utf8Stream::lowerCaseUtf8(item.getClass().getName(item));

            if (name.find(filter) == std::string::npos && !matchingEffectExists(filter, enchant->mEffects))
                continue;

            Spell newSpell;
            newSpell.mItem = item;
            newSpell.mId = item.getCellRef().getRefId();
            newSpell.mName = item.getClass().getName(item);
            newSpell.mCount = item.getCellRef().getCount();
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

                int castCost = MWMechanics::getEffectiveEnchantmentCastCost(*enchant, mActor);

                std::string cost = std::to_string(castCost);
                int currentCharge = int(item.getCellRef().getEnchantmentCharge());
                if (currentCharge == -1)
                    currentCharge = MWMechanics::getEnchantmentCharge(*enchant);
                std::string charge = std::to_string(currentCharge);
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

    SpellModel::ModelIndex SpellModel::getSelectedIndex() const
    {
        ModelIndex selected = -1;
        for (SpellModel::ModelIndex i = 0; i < int(getItemCount()); ++i)
        {
            if (getItem(i).mSelected)
            {
                selected = i;
                break;
            }
        }
        return selected;
    }

    Spell SpellModel::getItem(ModelIndex index) const
    {
        if (index < 0 || index >= int(mSpells.size()))
            throw std::runtime_error("invalid spell index supplied");
        return mSpells[index];
    }

}
