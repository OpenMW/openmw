
#include "inventorystore.hpp"

#include <iterator>
#include <algorithm>

#include <components/esm/loadench.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"
#include "../mwbase/mechanicsmanager.hpp"

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

int MWWorld::InventoryStore::getSlot (const MWWorld::LiveCellRefBase& ref) const
{
    for (int i = 0; i<static_cast<int> (mSlots.size()); ++i)
        if (mSlots[i].getType()!=-1 && mSlots[i]->getBase()==&ref)
            return i;

    return -1;
}

void MWWorld::InventoryStore::setSlot (const MWWorld::ContainerStoreIterator& iter, int slot)
{
    if (iter!=end() && slot>=0 && slot<Slots)
        mSlots[slot] = iter;
}

MWWorld::InventoryStore::InventoryStore()
 : mSelectedEnchantItem(end())
 , mUpdatesEnabled (true)
 , mFirstAutoEquip(true)
 , mListener(NULL)
{
    initSlots (mSlots);
}

MWWorld::InventoryStore::InventoryStore (const InventoryStore& store)
: ContainerStore (store)
 , mSelectedEnchantItem(end())
{
    mMagicEffects = store.mMagicEffects;
    mFirstAutoEquip = store.mFirstAutoEquip;
    mListener = store.mListener;
    mUpdatesEnabled = store.mUpdatesEnabled;

    mPermanentMagicEffectMagnitudes = store.mPermanentMagicEffectMagnitudes;
    copySlots (store);
}

MWWorld::InventoryStore& MWWorld::InventoryStore::operator= (const InventoryStore& store)
{
    mListener = store.mListener;
    mMagicEffects = store.mMagicEffects;
    mFirstAutoEquip = store.mFirstAutoEquip;
    mPermanentMagicEffectMagnitudes = store.mPermanentMagicEffectMagnitudes;
    ContainerStore::operator= (store);
    mSlots.clear();
    copySlots (store);
    return *this;
}

MWWorld::ContainerStoreIterator MWWorld::InventoryStore::add(const Ptr& itemPtr, int count, const Ptr& actorPtr, bool setOwner)
{
    const MWWorld::ContainerStoreIterator& retVal = MWWorld::ContainerStore::add(itemPtr, count, actorPtr, setOwner);

    // Auto-equip items if an armor/clothing or weapon item is added, but not for the player nor werewolves
    if ((actorPtr.getRefData().getHandle() != "player")
            && !(actorPtr.getClass().isNpc() && actorPtr.getClass().getNpcStats(actorPtr).isWerewolf())
            && !actorPtr.getClass().getCreatureStats(actorPtr).isDead())
    {
        std::string type = itemPtr.getTypeName();
        if ((type == typeid(ESM::Armor).name()) || (type == typeid(ESM::Clothing).name()) || (type == typeid(ESM::Weapon).name()))
            autoEquip(actorPtr);
    }

    updateRechargingItems();

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

    fireEquipmentChangedEvent();
    updateMagicEffects(actor);
}

void MWWorld::InventoryStore::unequipAll(const MWWorld::Ptr& actor)
{
    // Only *one* change event should be fired
    mUpdatesEnabled = false;
    for (int slot=0; slot < MWWorld::InventoryStore::Slots; ++slot)
        unequipSlot(slot, actor);
    mUpdatesEnabled = true;
    fireEquipmentChangedEvent();
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

        // Don't auto-equip probes or lockpicks. NPCs can't use them (yet). And AiCombat would attempt to "attack" with them.
        // NOTE: In the future AiCombat should handle equipping appropriate weapons
        if (test.getTypeName() == typeid(ESM::Lockpick).name() || test.getTypeName() == typeid(ESM::Probe).name())
            continue;

        // Only autoEquip if we are the original owner of the item.
        // This stops merchants from auto equipping anything you sell to them.
        // ...unless this is a companion, he should always equip items given to him.
        if (!Misc::StringUtils::ciEqual(test.getCellRef().getOwner(), actor.getCellRef().getRefId()) &&
                (actor.getClass().getScript(actor).empty() ||
                !actor.getRefData().getLocals().getIntVar(actor.getClass().getScript(actor), "companion")))
            continue;

        int testSkill = test.getClass().getEquipmentSkill (test);

        std::pair<std::vector<int>, bool> itemsSlots =
            iter->getClass().getEquipmentSlots (*iter);

        for (std::vector<int>::const_iterator iter2 (itemsSlots.first.begin());
            iter2!=itemsSlots.first.end(); ++iter2)
        {
            bool use = false;

            if (slots_.at (*iter2)==end())
                use = true; // slot was empty before -> skip all further checks
            else
            {
                Ptr old = *slots_.at (*iter2);

                if (!use)
                {
                    // check skill
                    int oldSkill =
                        old.getClass().getEquipmentSkill (old);

                    if (testSkill!=-1 && oldSkill==-1)
                        use = true;
                    else if (testSkill!=-1 && oldSkill!=-1 && testSkill!=oldSkill)
                    {
                        if (actor.getClass().getSkill(actor, oldSkill) > actor.getClass().getSkill (actor, testSkill))
                            continue; // rejected, because old item better matched the NPC's skills.

                        if (actor.getClass().getSkill(actor, oldSkill) < actor.getClass().getSkill (actor, testSkill))
                            use = true;
                    }
                }

                if (!use)
                {
                    // check value
                    if (old.getClass().getValue (old)>=
                        test.getClass().getValue (test))
                    {
                        continue;
                    }

                    use = true;
                }
            }

            switch(test.getClass().canBeEquipped (test, actor).first)
            {
                case 0:
                    continue;
                case 2:
                    slots_[MWWorld::InventoryStore::Slot_CarriedLeft] = end();
                    break;
                case 3:
                    // Prefer keeping twohanded weapon
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
        if (slots_[i]!=mSlots[i])
        {
            changed = true;
        }

    mUpdatesEnabled = true;

    if (changed)
    {
        mSlots.swap (slots_);
        fireEquipmentChangedEvent();
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
                    params[i].mRandom = static_cast<float> (std::rand()) / RAND_MAX;

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
    if ((actor.getRefData().getHandle() != "player")
            && !(actor.getClass().isNpc() && actor.getClass().getNpcStats(actor).isWerewolf()))
    {
        std::string type = item.getTypeName();
        if (((type == typeid(ESM::Armor).name()) || (type == typeid(ESM::Clothing).name()))
                && !actor.getClass().getCreatureStats(actor).isDead())
            autoEquip(actor);
    }

    if (item.getRefData().getCount() == 0 && mSelectedEnchantItem != end()
            && *mSelectedEnchantItem == item && actor.getRefData().getHandle() == "player")
    {
        mSelectedEnchantItem = end();
    }

    updateRechargingItems();

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

            if (actor.getRefData().getHandle() == "player")
            {
                // Unset OnPCEquip Variable on item's script, if it has a script with that variable declared
                const std::string& script = it->getClass().getScript(*it);
                if (script != "")
                    (*it).getRefData().getLocals().setVarByInt(script, "onpcequip", 0);

                if ((mSelectedEnchantItem != end()) && (mSelectedEnchantItem == it))
                {
                    mSelectedEnchantItem = end();
                }
            }
        }

        fireEquipmentChangedEvent();
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

void MWWorld::InventoryStore::fireEquipmentChangedEvent()
{
    if (!mUpdatesEnabled)
        return;
    if (mListener)
        mListener->equipmentChanged();
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
            visitor.visit(MWMechanics::EffectKey(*effectIt), (**iter).getClass().getName(**iter), -1, magnitude);

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
                mRechargingItems.push_back(std::make_pair(it, enchantment->mData.mCharge));
        }
    }
}

void MWWorld::InventoryStore::rechargeItems(float duration)
{
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
    mMagicEffects.add(MWMechanics::EffectKey(effectId), -mMagicEffects.get(MWMechanics::EffectKey(effectId)).mMagnitude);
}

void MWWorld::InventoryStore::clear()
{
    mSlots.clear();
    initSlots (mSlots);
    ContainerStore::clear();
}
