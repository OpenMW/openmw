
#include "inventorystore.hpp"

#include <iterator>
#include <algorithm>

#include <components/esm/loadench.hpp>
#include <components/esm/inventorystate.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"
#include "../mwbase/mechanicsmanager.hpp"
#include "../mwbase/windowmanager.hpp"

#include "../mwgui/inventorywindow.hpp"

#include "../mwmechanics/npcstats.hpp"
#include "../mwmechanics/spellcasting.hpp"


#include "esmstore.hpp"
#include "class.hpp"

void MWWorld::InventoryStore::copySlots (const InventoryStore& store)
{
    // some const-trickery, required because of a flaw in the handling of MW-references and the
    // resulting workarounds
    for (std::vector<ContainerStoreIterator>::const_iterator iter (
        const_cast<InventoryStore&> (store).mSlots.begin());
        iter!=const_cast<InventoryStore&> (store).mSlots.end(); ++iter)
    {
        std::size_t distance = std::distance (const_cast<InventoryStore&> (store).begin(), *iter);

        ContainerStoreIterator slot = begin();

        std::advance (slot, distance);

        mSlots.push_back (slot);
    }

    // some const-trickery, required because of a flaw in the handling of MW-references and the
    // resulting workarounds
    std::size_t distance = std::distance (const_cast<InventoryStore&> (store).begin(), const_cast<InventoryStore&> (store).mSelectedEnchantItem);
    ContainerStoreIterator slot = begin();
    std::advance (slot, distance);
    mSelectedEnchantItem = slot;
}

void MWWorld::InventoryStore::initSlots (TSlots& slots_)
{
    for (int i=0; i<Slots; ++i)
        slots_.push_back (end());
}

void MWWorld::InventoryStore::storeEquipmentState(const MWWorld::LiveCellRefBase &ref, int index, ESM::InventoryState &inventory) const
{
    for (int i = 0; i<static_cast<int> (mSlots.size()); ++i)
        if (mSlots[i].getType()!=-1 && mSlots[i]->getBase()==&ref)
        {
            inventory.mEquipmentSlots[index] = i;
        }

    if (mSelectedEnchantItem.getType()!=-1 && mSelectedEnchantItem->getBase() == &ref)
        inventory.mSelectedEnchantItem = index;
}

void MWWorld::InventoryStore::readEquipmentState(const MWWorld::ContainerStoreIterator &iter, int index, const ESM::InventoryState &inventory)
{
    if (index == inventory.mSelectedEnchantItem)
        mSelectedEnchantItem = iter;

    std::map<int, int>::const_iterator found = inventory.mEquipmentSlots.find(index);
    if (found != inventory.mEquipmentSlots.end())
    {
        if (found->second < 0 || found->second >= MWWorld::InventoryStore::Slots)
            throw std::runtime_error("Invalid slot index in inventory state");

        // make sure the item can actually be equipped in this slot
        int slot = found->second;
        std::pair<std::vector<int>, bool> allowedSlots = iter->getClass().getEquipmentSlots(*iter);
        if (!allowedSlots.first.size())
            return;
        if (std::find(allowedSlots.first.begin(), allowedSlots.first.end(), slot) == allowedSlots.first.end())
            slot = allowedSlots.first.front();

        // unstack if required
        if (!allowedSlots.second && iter->getRefData().getCount() > 1)
        {
            MWWorld::ContainerStoreIterator newIter = addNewStack(*iter, 1);
            iter->getRefData().setCount(iter->getRefData().getCount()-1);
            mSlots[slot] = newIter;
        }
        else
            mSlots[slot] = iter;
    }
}

MWWorld::InventoryStore::InventoryStore()
 : mSelectedEnchantItem(end())
 , mUpdatesEnabled (true)
 , mFirstAutoEquip(true)
 , mListener(NULL)
 , mRechargingItemsUpToDate(false)
{
    initSlots (mSlots);
}

MWWorld::InventoryStore::InventoryStore (const InventoryStore& store)
 : ContainerStore (store)
 , mSelectedEnchantItem(end())
 , mMagicEffects(store.mMagicEffects)
 , mFirstAutoEquip(store.mFirstAutoEquip)
 , mListener(store.mListener)
 , mUpdatesEnabled(store.mUpdatesEnabled)
 , mPermanentMagicEffectMagnitudes(store.mPermanentMagicEffectMagnitudes)
 , mRechargingItemsUpToDate(false)
{
    copySlots (store);
}

MWWorld::InventoryStore& MWWorld::InventoryStore::operator= (const InventoryStore& store)
{
    mListener = store.mListener;
    mMagicEffects = store.mMagicEffects;
    mFirstAutoEquip = store.mFirstAutoEquip;
    mPermanentMagicEffectMagnitudes = store.mPermanentMagicEffectMagnitudes;
    mRechargingItemsUpToDate = false;
    ContainerStore::operator= (store);
    mSlots.clear();
    copySlots (store);
    return *this;
}

MWWorld::ContainerStoreIterator MWWorld::InventoryStore::add(const Ptr& itemPtr, int count, const Ptr& actorPtr, bool setOwner)
{
    const MWWorld::ContainerStoreIterator& retVal = MWWorld::ContainerStore::add(itemPtr, count, actorPtr, setOwner);

    // Auto-equip items if an armor/clothing or weapon item is added, but not for the player nor werewolves
    if (actorPtr != MWBase::Environment::get().getWorld()->getPlayerPtr()
            && !(actorPtr.getClass().isNpc() && actorPtr.getClass().getNpcStats(actorPtr).isWerewolf()))
    {
        std::string type = itemPtr.getTypeName();
        if (type == typeid(ESM::Armor).name() || type == typeid(ESM::Clothing).name())
            autoEquip(actorPtr);
    }

    return retVal;
}

void MWWorld::InventoryStore::equip (int slot, const ContainerStoreIterator& iterator, const Ptr& actor)
{
    if (iterator == end())
        throw std::runtime_error ("can't equip end() iterator, use unequip function instead");

    if (slot<0 || slot>=static_cast<int> (mSlots.size()))
        throw std::runtime_error ("slot number out of range");

    if (iterator.getContainerStore()!=this)
        throw std::runtime_error ("attempt to equip an item that is not in the inventory");

    std::pair<std::vector<int>, bool> slots_;

    slots_ = iterator->getClass().getEquipmentSlots (*iterator);

    if (std::find (slots_.first.begin(), slots_.first.end(), slot)==slots_.first.end())
        throw std::runtime_error ("invalid slot");

    if (mSlots[slot] != end())
        unequipSlot(slot, actor);

    // unstack item pointed to by iterator if required
    if (iterator!=end() && !slots_.second && iterator->getRefData().getCount() > 1) // if slots.second is true, item can stay stacked when equipped
    {
        unstack(*iterator, actor);
    }

    mSlots[slot] = iterator;

    flagAsModified();

    fireEquipmentChangedEvent(actor);

    updateMagicEffects(actor);
}

void MWWorld::InventoryStore::unequipAll(const MWWorld::Ptr& actor)
{
    mUpdatesEnabled = false;
    for (int slot=0; slot < MWWorld::InventoryStore::Slots; ++slot)
        unequipSlot(slot, actor);

    mUpdatesEnabled = true;

    fireEquipmentChangedEvent(actor);
    updateMagicEffects(actor);
}

MWWorld::ContainerStoreIterator MWWorld::InventoryStore::getSlot (int slot)
{
    if (slot<0 || slot>=static_cast<int> (mSlots.size()))
        throw std::runtime_error ("slot number out of range");

    if (mSlots[slot]==end())
        return end();

    if (mSlots[slot]->getRefData().getCount()<1)
    {
        // Object has been deleted
        // This should no longer happen, since the new remove function will unequip first
        throw std::runtime_error("Invalid slot, make sure you are not calling RefData::setCount for a container object");
    }

    return mSlots[slot];
}

void MWWorld::InventoryStore::autoEquip (const MWWorld::Ptr& actor)
{
    TSlots slots_;
    initSlots (slots_);

    // Disable model update during auto-equip
    mUpdatesEnabled = false;

    for (ContainerStoreIterator iter (begin()); iter!=end(); ++iter)
    {
        Ptr test = *iter;

        // Don't autoEquip lights. Handled in Actors::updateEquippedLight based on environment light.
        if (test.getTypeName() == typeid(ESM::Light).name())
        {
            continue;
        }

        // Only autoEquip if we are the original owner of the item.
        // This stops merchants from auto equipping anything you sell to them.
        // ...unless this is a companion, he should always equip items given to him.
        if (!Misc::StringUtils::ciEqual(test.getCellRef().getOwner(), actor.getCellRef().getRefId()) &&
                (actor.getClass().getScript(actor).empty() ||
                !actor.getRefData().getLocals().getIntVar(actor.getClass().getScript(actor), "companion"))
                && !actor.getClass().getCreatureStats(actor).isDead() // Corpses can be dressed up by the player as desired
                )
        {
            continue;
        }
        int testSkill = test.getClass().getEquipmentSkill (test);

        std::pair<std::vector<int>, bool> itemsSlots =
            iter->getClass().getEquipmentSlots (*iter);

        for (std::vector<int>::const_iterator iter2 (itemsSlots.first.begin());
            iter2!=itemsSlots.first.end(); ++iter2)
        {
            if (*iter2 == Slot_CarriedRight) // Items in right hand are situational use, so don't equip them.
                // Equipping weapons is handled by AiCombat. Anything else (lockpicks, probes) can't be used by NPCs anyway (yet)
                continue;

            if (slots_.at (*iter2)!=end())
            {
                Ptr old = *slots_.at (*iter2);

                // check skill
                int oldSkill = old.getClass().getEquipmentSkill (old);

                bool use = false;
                if (testSkill!=-1 && oldSkill==-1)
                    use = true;
                else if (testSkill!=-1 && oldSkill!=-1 && testSkill!=oldSkill)
                {
                    if (actor.getClass().getSkill(actor, oldSkill) > actor.getClass().getSkill (actor, testSkill))
                        continue; // rejected, because old item better matched the NPC's skills.

                    if (actor.getClass().getSkill(actor, oldSkill) < actor.getClass().getSkill (actor, testSkill))
                        use = true;
                }

                if (!use)
                {
                    // check value
                    if (old.getClass().getValue (old)>=
                        test.getClass().getValue (test))
                    {
                        continue;
                    }
                }
            }

            switch(test.getClass().canBeEquipped (test, actor).first)
            {
                case 0:
                    continue;
                default:
                    break;
            }

            if (!itemsSlots.second) // if itemsSlots.second is true, item can stay stacked when equipped
            {
                // unstack item pointed to by iterator if required
                if (iter->getRefData().getCount() > 1)
                {
                    unstack(*iter, actor);
                }
            }

            slots_[*iter2] = iter;
            break;
        }
    }

    bool changed = false;

    for (std::size_t i=0; i<slots_.size(); ++i)
    {
        if (slots_[i] != mSlots[i])
        {
            changed = true;
            break;
        }
    }
    mUpdatesEnabled = true;

    if (changed)
    {
        mSlots.swap (slots_);
        fireEquipmentChangedEvent(actor);
        updateMagicEffects(actor);
        flagAsModified();
    }
}

const MWMechanics::MagicEffects& MWWorld::InventoryStore::getMagicEffects() const
{
    return mMagicEffects;
}

void MWWorld::InventoryStore::updateMagicEffects(const Ptr& actor)
{
    // To avoid excessive updates during auto-equip
    if (!mUpdatesEnabled)
        return;

    // Delay update until the listener is set up
    if (!mListener)
        return;

    mMagicEffects = MWMechanics::MagicEffects();

    for (TSlots::const_iterator iter (mSlots.begin()); iter!=mSlots.end(); ++iter)
    {
        if (*iter==end())
            continue;

        std::string enchantmentId = (*iter)->getClass().getEnchantment (**iter);

        if (!enchantmentId.empty())
        {
            const ESM::Enchantment& enchantment =
                *MWBase::Environment::get().getWorld()->getStore().get<ESM::Enchantment>().find (enchantmentId);

            if (enchantment.mData.mType != ESM::Enchantment::ConstantEffect)
                continue;

            std::vector<EffectParams> params;

            bool existed = (mPermanentMagicEffectMagnitudes.find((**iter).getCellRef().getRefId()) != mPermanentMagicEffectMagnitudes.end());
            if (!existed)
            {
                // Roll some dice, one for each effect
                params.resize(enchantment.mEffects.mList.size());
                for (unsigned int i=0; i<params.size();++i)
                    params[i].mRandom = OEngine::Misc::Rng::rollClosedProbability();

                // Try resisting each effect
                int i=0;
                for (std::vector<ESM::ENAMstruct>::const_iterator effectIt (enchantment.mEffects.mList.begin());
                    effectIt!=enchantment.mEffects.mList.end(); ++effectIt)
                {
                    params[i].mMultiplier = MWMechanics::getEffectMultiplier(effectIt->mEffectID, actor, actor);
                    ++i;
                }

                // Note that using the RefID as a key here is not entirely correct.
                // Consider equipping the same item twice (e.g. a ring)
                // However, permanent enchantments with a random magnitude are kind of an exploit anyway,
                // so it doesn't really matter if both items will get the same magnitude. *Extreme* edge case.
                mPermanentMagicEffectMagnitudes[(**iter).getCellRef().getRefId()] = params;
            }
            else
                params = mPermanentMagicEffectMagnitudes[(**iter).getCellRef().getRefId()];

            int i=0;
            for (std::vector<ESM::ENAMstruct>::const_iterator effectIt (enchantment.mEffects.mList.begin());
                effectIt!=enchantment.mEffects.mList.end(); ++effectIt, ++i)
            {
                const ESM::MagicEffect *magicEffect =
                    MWBase::Environment::get().getWorld()->getStore().get<ESM::MagicEffect>().find (
                    effectIt->mEffectID);

                // Fully resisted?
                if (params[i].mMultiplier == 0)
                    continue;

                float magnitude = effectIt->mMagnMin + (effectIt->mMagnMax - effectIt->mMagnMin) * params[i].mRandom;
                magnitude *= params[i].mMultiplier;

                if (!existed)
                {
                    // During first auto equip, we don't play any sounds.
                    // Basically we don't want sounds when the actor is first loaded,
                    // the items should appear as if they'd always been equipped.
                    mListener->permanentEffectAdded(magicEffect, !mFirstAutoEquip,
                                                        !mFirstAutoEquip && effectIt == enchantment.mEffects.mList.begin());

                    // Apply instant effects
                    MWMechanics::CastSpell cast(actor, actor);
                    if (magnitude)
                        cast.applyInstantEffect(actor, actor, effectIt->mEffectID, magnitude);
                }

                if (magnitude)
                    mMagicEffects.add (*effectIt, magnitude);
            }
        }
    }

    // Now drop expired effects
    for (TEffectMagnitudes::iterator it = mPermanentMagicEffectMagnitudes.begin();
         it != mPermanentMagicEffectMagnitudes.end();)
    {
        bool found = false;
        for (TSlots::const_iterator iter (mSlots.begin()); iter!=mSlots.end(); ++iter)
        {
            if (*iter == end())
                continue;
            if ((**iter).getCellRef().getRefId() == it->first)
            {
                found = true;
            }
        }
        if (!found)
            mPermanentMagicEffectMagnitudes.erase(it++);
        else
            ++it;
    }

    // Magic effects are normally not updated when paused, but we need this to make resistances work immediately after equipping
    MWBase::Environment::get().getMechanicsManager()->updateMagicEffects(actor);

    mFirstAutoEquip = false;
}

void MWWorld::InventoryStore::flagAsModified()
{
    ContainerStore::flagAsModified();
    mRechargingItemsUpToDate = false;
}

bool MWWorld::InventoryStore::stacks(const Ptr& ptr1, const Ptr& ptr2)
{
    bool canStack = MWWorld::ContainerStore::stacks(ptr1, ptr2);
    if (!canStack)
        return false;

    // don't stack if either item is currently equipped
    for (TSlots::const_iterator iter (mSlots.begin());
        iter!=mSlots.end(); ++iter)
    {
        if (*iter != end() && (ptr1 == **iter || ptr2 == **iter))
        {
            bool stackWhenEquipped = (*iter)->getClass().getEquipmentSlots(**iter).second;
            if (!stackWhenEquipped)
                return false;
        }
    }

    return true;
}

void MWWorld::InventoryStore::setSelectedEnchantItem(const ContainerStoreIterator& iterator)
{
    mSelectedEnchantItem = iterator;
}

MWWorld::ContainerStoreIterator MWWorld::InventoryStore::getSelectedEnchantItem()
{
    return mSelectedEnchantItem;
}

int MWWorld::InventoryStore::remove(const Ptr& item, int count, const Ptr& actor)
{
    int retCount = ContainerStore::remove(item, count, actor);

    if (!item.getRefData().getCount())
    {
        for (int slot=0; slot < MWWorld::InventoryStore::Slots; ++slot)
        {
            if (mSlots[slot] == end())
                continue;

            if (*mSlots[slot] == item)
            {
                unequipSlot(slot, actor);
                break;
            }
        }
    }

    // If an armor/clothing item is removed, try to find a replacement,
    // but not for the player nor werewolves.
    if ((actor != MWBase::Environment::get().getWorld()->getPlayerPtr())
            && !(actor.getClass().isNpc() && actor.getClass().getNpcStats(actor).isWerewolf()))
    {
        std::string type = item.getTypeName();
        if (type == typeid(ESM::Armor).name() || type == typeid(ESM::Clothing).name())
            autoEquip(actor);
    }

    if (item.getRefData().getCount() == 0 && mSelectedEnchantItem != end()
            && *mSelectedEnchantItem == item)
    {
        mSelectedEnchantItem = end();
    }

    return retCount;
}

MWWorld::ContainerStoreIterator MWWorld::InventoryStore::unequipSlot(int slot, const MWWorld::Ptr& actor)
{
    ContainerStoreIterator it = mSlots[slot];

    if (it != end())
    {
        ContainerStoreIterator retval = it;

        // empty this slot
        mSlots[slot] = end();

        if (it->getRefData().getCount())
        {
            retval = restack(*it);

            if (actor == MWBase::Environment::get().getWorld()->getPlayerPtr())
            {
                // Unset OnPCEquip Variable on item's script, if it has a script with that variable declared
                const std::string& script = it->getClass().getScript(*it);
                if (script != "")
                    (*it).getRefData().getLocals().setVarByInt(script, "onpcequip", 0);
            }

            if ((mSelectedEnchantItem != end()) && (mSelectedEnchantItem == it))
            {
                mSelectedEnchantItem = end();
            }
        }

        fireEquipmentChangedEvent(actor);
        updateMagicEffects(actor);

        return retval;
    }

    return it;
}

MWWorld::ContainerStoreIterator MWWorld::InventoryStore::unequipItem(const MWWorld::Ptr& item, const MWWorld::Ptr& actor)
{
    for (int slot=0; slot<MWWorld::InventoryStore::Slots; ++slot)
    {
        MWWorld::ContainerStoreIterator equipped = getSlot(slot);
        if (equipped != end() && *equipped == item)
            return unequipSlot(slot, actor);
    }

    throw std::runtime_error ("attempt to unequip an item that is not currently equipped");
}

void MWWorld::InventoryStore::setListener(InventoryStoreListener *listener, const Ptr& actor)
{
    mListener = listener;
    updateMagicEffects(actor);
}

void MWWorld::InventoryStore::fireEquipmentChangedEvent(const Ptr& actor)
{
    if (!mUpdatesEnabled)
        return;
    if (mListener)
        mListener->equipmentChanged();

    // if player, update inventory window
    if (actor == MWBase::Environment::get().getWorld()->getPlayerPtr())
    {
        MWBase::Environment::get().getWindowManager()->getInventoryWindow()->updateItemView();
    }
}

void MWWorld::InventoryStore::visitEffectSources(MWMechanics::EffectSourceVisitor &visitor)
{
    for (TSlots::const_iterator iter (mSlots.begin()); iter!=mSlots.end(); ++iter)
    {
        if (*iter==end())
            continue;

        std::string enchantmentId = (*iter)->getClass().getEnchantment (**iter);
        if (enchantmentId.empty())
            continue;

        const ESM::Enchantment& enchantment =
            *MWBase::Environment::get().getWorld()->getStore().get<ESM::Enchantment>().find (enchantmentId);

        if (enchantment.mData.mType != ESM::Enchantment::ConstantEffect)
            continue;

        if (mPermanentMagicEffectMagnitudes.find((**iter).getCellRef().getRefId()) == mPermanentMagicEffectMagnitudes.end())
            continue;

        int i=0;
        for (std::vector<ESM::ENAMstruct>::const_iterator effectIt (enchantment.mEffects.mList.begin());
            effectIt!=enchantment.mEffects.mList.end(); ++effectIt)
        {
            const EffectParams& params = mPermanentMagicEffectMagnitudes[(**iter).getCellRef().getRefId()][i];
            float magnitude = effectIt->mMagnMin + (effectIt->mMagnMax - effectIt->mMagnMin) * params.mRandom;
            magnitude *= params.mMultiplier;
            if (magnitude > 0)
                visitor.visit(MWMechanics::EffectKey(*effectIt), (**iter).getClass().getName(**iter), (**iter).getCellRef().getRefId(), -1, magnitude);

            ++i;
        }
    }
}

void MWWorld::InventoryStore::updateRechargingItems()
{
    mRechargingItems.clear();
    for (ContainerStoreIterator it = begin(); it != end(); ++it)
    {
        if (it->getClass().getEnchantment(*it) != "")
        {
            const ESM::Enchantment* enchantment = MWBase::Environment::get().getWorld()->getStore().get<ESM::Enchantment>().find(
                        it->getClass().getEnchantment(*it));
            if (enchantment->mData.mType == ESM::Enchantment::WhenUsed
                    || enchantment->mData.mType == ESM::Enchantment::WhenStrikes)
                mRechargingItems.push_back(std::make_pair(it, static_cast<float>(enchantment->mData.mCharge)));
        }
    }
}

void MWWorld::InventoryStore::rechargeItems(float duration)
{
    if (!mRechargingItemsUpToDate)
    {
        updateRechargingItems();
        mRechargingItemsUpToDate = true;
    }
    for (TRechargingItems::iterator it = mRechargingItems.begin(); it != mRechargingItems.end(); ++it)
    {
        if (it->first->getCellRef().getEnchantmentCharge() == -1
                || it->first->getCellRef().getEnchantmentCharge() == it->second)
            continue;

        static float fMagicItemRechargePerSecond = MWBase::Environment::get().getWorld()->getStore().get<ESM::GameSetting>().find(
                    "fMagicItemRechargePerSecond")->getFloat();

        if (it->first->getCellRef().getEnchantmentCharge() <= it->second)
        {
            it->first->getCellRef().setEnchantmentCharge(std::min (it->first->getCellRef().getEnchantmentCharge() + fMagicItemRechargePerSecond * duration,
                                                                  it->second));

            // attempt to restack when fully recharged
            if (it->first->getCellRef().getEnchantmentCharge() == it->second)
                it->first = restack(*it->first);
        }
    }
}

void MWWorld::InventoryStore::purgeEffect(short effectId)
{
    mMagicEffects.remove(MWMechanics::EffectKey(effectId));
}

void MWWorld::InventoryStore::purgeEffect(short effectId, const std::string &sourceId)
{
    TEffectMagnitudes::iterator effectMagnitudeIt = mPermanentMagicEffectMagnitudes.find(sourceId);
    if (effectMagnitudeIt == mPermanentMagicEffectMagnitudes.end())
        return;

    for (TSlots::const_iterator iter (mSlots.begin()); iter!=mSlots.end(); ++iter)
    {
        if (*iter==end())
            continue;

        if ((*iter)->getClass().getId(**iter) != sourceId)
            continue;

        std::string enchantmentId = (*iter)->getClass().getEnchantment (**iter);

        if (!enchantmentId.empty())
        {
            const ESM::Enchantment& enchantment =
                *MWBase::Environment::get().getWorld()->getStore().get<ESM::Enchantment>().find (enchantmentId);

            if (enchantment.mData.mType != ESM::Enchantment::ConstantEffect)
                continue;

            std::vector<EffectParams>& params = effectMagnitudeIt->second;

            int i=0;
            for (std::vector<ESM::ENAMstruct>::const_iterator effectIt (enchantment.mEffects.mList.begin());
                effectIt!=enchantment.mEffects.mList.end(); ++effectIt, ++i)
            {
                if (effectIt->mEffectID != effectId)
                    continue;

                float magnitude = effectIt->mMagnMin + (effectIt->mMagnMax - effectIt->mMagnMin) * params[i].mRandom;
                magnitude *= params[i].mMultiplier;

                if (magnitude)
                    mMagicEffects.add (*effectIt, -magnitude);

                params[i].mMultiplier = 0;
                break;
            }
        }
    }
}

void MWWorld::InventoryStore::clear()
{
    mSlots.clear();
    initSlots (mSlots);
    ContainerStore::clear();
}

bool MWWorld::InventoryStore::isEquipped(const MWWorld::Ptr &item)
{
    for (int i=0; i < MWWorld::InventoryStore::Slots; ++i)
    {
        if (getSlot(i) != end() && *getSlot(i) == item)
            return true;
    }
    return false;
}

void MWWorld::InventoryStore::writeState(ESM::InventoryState &state)
{
    MWWorld::ContainerStore::writeState(state);

    for (TEffectMagnitudes::const_iterator it = mPermanentMagicEffectMagnitudes.begin(); it != mPermanentMagicEffectMagnitudes.end(); ++it)
    {
        std::vector<std::pair<float, float> > params;
        for (std::vector<EffectParams>::const_iterator pIt = it->second.begin(); pIt != it->second.end(); ++pIt)
        {
            params.push_back(std::make_pair(pIt->mRandom, pIt->mMultiplier));
        }

        state.mPermanentMagicEffectMagnitudes[it->first] = params;
    }
}

void MWWorld::InventoryStore::readState(const ESM::InventoryState &state)
{
    MWWorld::ContainerStore::readState(state);

    for (ESM::InventoryState::TEffectMagnitudes::const_iterator it = state.mPermanentMagicEffectMagnitudes.begin();
         it != state.mPermanentMagicEffectMagnitudes.end(); ++it)
    {
        std::vector<EffectParams> params;
        for (std::vector<std::pair<float, float> >::const_iterator pIt = it->second.begin(); pIt != it->second.end(); ++pIt)
        {
            EffectParams p;
            p.mRandom = pIt->first;
            p.mMultiplier = pIt->second;
            params.push_back(p);
        }

        mPermanentMagicEffectMagnitudes[it->first] = params;
    }
}
