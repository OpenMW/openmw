#include "inventorystore.hpp"

#include <iterator>
#include <algorithm>

#include <components/debug/debuglog.hpp>
#include <components/esm/loadench.hpp>
#include <components/esm/inventorystate.hpp>
#include <components/misc/rng.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"
#include "../mwbase/mechanicsmanager.hpp"

#include "../mwmechanics/npcstats.hpp"
#include "../mwmechanics/spellcasting.hpp"
#include "../mwmechanics/actorutil.hpp"
#include "../mwmechanics/weapontype.hpp"

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
 : ContainerStore()
 , mInventoryListener(nullptr)
 , mUpdatesEnabled (true)
 , mFirstAutoEquip(true)
 , mSelectedEnchantItem(end())
{
    initSlots (mSlots);
}

MWWorld::InventoryStore::InventoryStore (const InventoryStore& store)
 : ContainerStore (store)
 , mMagicEffects(store.mMagicEffects)
 , mInventoryListener(store.mInventoryListener)
 , mUpdatesEnabled(store.mUpdatesEnabled)
 , mFirstAutoEquip(store.mFirstAutoEquip)
 , mPermanentMagicEffectMagnitudes(store.mPermanentMagicEffectMagnitudes)
 , mSelectedEnchantItem(end())
{
    copySlots (store);
}

MWWorld::InventoryStore& MWWorld::InventoryStore::operator= (const InventoryStore& store)
{
    mListener = store.mListener;
    mInventoryListener = store.mInventoryListener;
    mMagicEffects = store.mMagicEffects;
    mFirstAutoEquip = store.mFirstAutoEquip;
    mPermanentMagicEffectMagnitudes = store.mPermanentMagicEffectMagnitudes;
    mRechargingItemsUpToDate = false;
    ContainerStore::operator= (store);
    mSlots.clear();
    copySlots (store);
    return *this;
}

MWWorld::ContainerStoreIterator MWWorld::InventoryStore::add(const Ptr& itemPtr, int count, const Ptr& actorPtr, bool allowAutoEquip)
{
    const MWWorld::ContainerStoreIterator& retVal = MWWorld::ContainerStore::add(itemPtr, count, actorPtr, allowAutoEquip);

    // Auto-equip items if an armor/clothing item is added, but not for the player nor werewolves
    if (allowAutoEquip && actorPtr != MWMechanics::getPlayer()
            && actorPtr.getClass().isNpc() && !actorPtr.getClass().getNpcStats(actorPtr).isWerewolf())
    {
        std::string type = itemPtr.getTypeName();
        if (type == typeid(ESM::Armor).name() || type == typeid(ESM::Clothing).name())
            autoEquip(actorPtr);
    }

    if (mListener)
        mListener->itemAdded(*retVal, count);

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
    return findSlot (slot);
}

MWWorld::ConstContainerStoreIterator MWWorld::InventoryStore::getSlot (int slot) const
{
    return findSlot (slot);
}

MWWorld::ContainerStoreIterator MWWorld::InventoryStore::findSlot (int slot) const
{
    if (slot<0 || slot>=static_cast<int> (mSlots.size()))
        throw std::runtime_error ("slot number out of range");

    if (mSlots[slot]==end())
        return mSlots[slot];

    if (mSlots[slot]->getRefData().getCount()<1)
    {
        // Object has been deleted
        // This should no longer happen, since the new remove function will unequip first
        throw std::runtime_error("Invalid slot, make sure you are not calling RefData::setCount for a container object");
    }

    return mSlots[slot];
}

void MWWorld::InventoryStore::autoEquipWeapon (const MWWorld::Ptr& actor, TSlots& slots_)
{
    if (!actor.getClass().isNpc())
    {
        // In original game creatures do not autoequip weapon, but we need it for weapon sheathing.
        // The only case when the difference is noticable - when this creature sells weapon.
        // So just disable weapon autoequipping for creatures which sells weapon.
        int services = actor.getClass().getServices(actor);
        bool sellsWeapon = services & (ESM::NPC::Weapon|ESM::NPC::MagicItems);
        if (sellsWeapon)
            return;
    }

    static const ESM::Skill::SkillEnum weaponSkills[] =
    {
        ESM::Skill::LongBlade,
        ESM::Skill::Axe,
        ESM::Skill::Spear,
        ESM::Skill::ShortBlade,
        ESM::Skill::Marksman,
        ESM::Skill::BluntWeapon
    };
    const size_t weaponSkillsLength = sizeof(weaponSkills) / sizeof(weaponSkills[0]);

    bool weaponSkillVisited[weaponSkillsLength] = { false };

    // give arrows/bolt with max damage by default
    int arrowMax = 0;
    int boltMax = 0;
    ContainerStoreIterator arrow(end());
    ContainerStoreIterator bolt(end());

    // rate ammo
    for (ContainerStoreIterator iter(begin(ContainerStore::Type_Weapon)); iter!=end(); ++iter)
    {
        const ESM::Weapon* esmWeapon = iter->get<ESM::Weapon>()->mBase;

        if (esmWeapon->mData.mType == ESM::Weapon::Arrow)
        {
            if (esmWeapon->mData.mChop[1] >= arrowMax)
            {
                arrowMax = esmWeapon->mData.mChop[1];
                arrow = iter;
            }
        }
        else if (esmWeapon->mData.mType == ESM::Weapon::Bolt)
        {
            if (esmWeapon->mData.mChop[1] >= boltMax)
            {
                boltMax = esmWeapon->mData.mChop[1];
                bolt = iter;
            }
        }
    }

    // rate weapon
    for (int i = 0; i < static_cast<int>(weaponSkillsLength); ++i)
    {
        int max = 0;
        int maxWeaponSkill = -1;

        for (int j = 0; j < static_cast<int>(weaponSkillsLength); ++j)
        {
            int skillValue = actor.getClass().getSkill(actor, static_cast<int>(weaponSkills[j]));
            if (skillValue > max && !weaponSkillVisited[j])
            {
                max = skillValue;
                maxWeaponSkill = j;
            }
        }

        if (maxWeaponSkill == -1)
            break;

        max = 0;
        ContainerStoreIterator weapon(end());

        for (ContainerStoreIterator iter(begin(ContainerStore::Type_Weapon)); iter!=end(); ++iter)
        {
            const ESM::Weapon* esmWeapon = iter->get<ESM::Weapon>()->mBase;

            if (MWMechanics::getWeaponType(esmWeapon->mData.mType)->mWeaponClass == ESM::WeaponType::Ammo)
                continue;

            if (iter->getClass().getEquipmentSkill(*iter) == weaponSkills[maxWeaponSkill])
            {
                if (esmWeapon->mData.mChop[1] >= max)
                {
                    max = esmWeapon->mData.mChop[1];
                    weapon = iter;
                }

                if (esmWeapon->mData.mSlash[1] >= max)
                {
                    max = esmWeapon->mData.mSlash[1];
                    weapon = iter;
                }

                if (esmWeapon->mData.mThrust[1] >= max)
                {
                    max = esmWeapon->mData.mThrust[1];
                    weapon = iter;
                }
            }
        }

        if (weapon != end() && weapon->getClass().canBeEquipped(*weapon, actor).first)
        {
            // Do not equip ranged weapons, if there is no suitable ammo
            bool hasAmmo = true;
            const MWWorld::LiveCellRef<ESM::Weapon> *ref = weapon->get<ESM::Weapon>();
            int type = ref->mBase->mData.mType;
            int ammotype = MWMechanics::getWeaponType(type)->mAmmoType;
            if (ammotype == ESM::Weapon::Arrow)
            {
                if (arrow == end())
                    hasAmmo = false;
                else
                    slots_[Slot_Ammunition] = arrow;
            }
            else if (ammotype == ESM::Weapon::Bolt)
            {
                if (bolt == end())
                    hasAmmo = false;
                else
                    slots_[Slot_Ammunition] = bolt;
            }

            if (hasAmmo)
            {
                std::pair<std::vector<int>, bool> itemsSlots = weapon->getClass().getEquipmentSlots (*weapon);

                if (!itemsSlots.first.empty())
                {
                    if (!itemsSlots.second)
                    {
                        if (weapon->getRefData().getCount() > 1)
                        {
                            unstack(*weapon, actor);
                        }
                    }

                    int slot = itemsSlots.first.front();
                    slots_[slot] = weapon;

                    if (ammotype == ESM::Weapon::None)
                        slots_[Slot_Ammunition] = end();
                }

                break;
            }
        }

        weaponSkillVisited[maxWeaponSkill] = true;
    }
}

void MWWorld::InventoryStore::autoEquipArmor (const MWWorld::Ptr& actor, TSlots& slots_)
{
    // Only NPCs can wear armor for now.
    // For creatures we equip only shields.
    if (!actor.getClass().isNpc())
    {
        autoEquipShield(actor, slots_);
        return;
    }

    const MWBase::World *world = MWBase::Environment::get().getWorld();
    const MWWorld::Store<ESM::GameSetting> &store = world->getStore().get<ESM::GameSetting>();

    static float fUnarmoredBase1 = store.find("fUnarmoredBase1")->mValue.getFloat();
    static float fUnarmoredBase2 = store.find("fUnarmoredBase2")->mValue.getFloat();

    int unarmoredSkill = actor.getClass().getSkill(actor, ESM::Skill::Unarmored);
    float unarmoredRating = (fUnarmoredBase1 * unarmoredSkill) * (fUnarmoredBase2 * unarmoredSkill);

    for (ContainerStoreIterator iter (begin(ContainerStore::Type_Clothing | ContainerStore::Type_Armor)); iter!=end(); ++iter)
    {
        Ptr test = *iter;

        switch(test.getClass().canBeEquipped (test, actor).first)
        {
            case 0:
                continue;
            default:
                break;
        }

        if (iter.getType() == ContainerStore::Type_Armor &&
                test.getClass().getEffectiveArmorRating(test, actor) <= std::max(unarmoredRating, 0.f))
        {
            continue;
        }

        std::pair<std::vector<int>, bool> itemsSlots =
            iter->getClass().getEquipmentSlots (*iter);

        // checking if current item pointed by iter can be equipped
        for (int slot : itemsSlots.first)
        {
            // if true then it means slot is equipped already
            // check if slot may require swapping if current item is more valuable
            if (slots_.at (slot)!=end())
            {
                Ptr old = *slots_.at (slot);

                if (iter.getType() == ContainerStore::Type_Armor)
                {
                    if (old.getTypeName() == typeid(ESM::Armor).name())
                    {
                        if (old.get<ESM::Armor>()->mBase->mData.mType < test.get<ESM::Armor>()->mBase->mData.mType)
                            continue;

                        if (old.get<ESM::Armor>()->mBase->mData.mType == test.get<ESM::Armor>()->mBase->mData.mType)
                        {
                            if (old.getClass().getEffectiveArmorRating(old, actor) >= test.getClass().getEffectiveArmorRating(test, actor))
                                // old armor had better armor rating
                                continue;
                        }
                    }
                    // suitable armor should replace already equipped clothing
                }
                else if (iter.getType() == ContainerStore::Type_Clothing)
                {
                    // if left ring is equipped
                    if (slot == Slot_LeftRing)
                    {
                        // if there is a place for right ring dont swap it
                        if (slots_.at(Slot_RightRing) == end())
                        {
                            continue;
                        }
                        else // if right ring is equipped too
                        {
                            Ptr rightRing = *slots_.at(Slot_RightRing);

                            // we want to swap cheaper ring only if both are equipped
                            if (old.getClass().getValue (old) >= rightRing.getClass().getValue (rightRing))
                                continue;
                        }
                    }

                    if (old.getTypeName() == typeid(ESM::Clothing).name())
                    {
                        // check value
                        if (old.getClass().getValue (old) >= test.getClass().getValue (test))
                            // old clothing was more valuable
                            continue;
                    }
                    else
                        // suitable clothing should NOT replace already equipped armor
                        continue;
                }
            }

            if (!itemsSlots.second) // if itemsSlots.second is true, item can stay stacked when equipped
            {
                // unstack item pointed to by iterator if required
                if (iter->getRefData().getCount() > 1)
                {
                    unstack(*iter, actor);
                }
            }

            // if we are here it means item can be equipped or swapped
            slots_[slot] = iter;
            break;
        }
    }
}

void MWWorld::InventoryStore::autoEquipShield(const MWWorld::Ptr& actor, TSlots& slots_)
{
    for (ContainerStoreIterator iter(begin(ContainerStore::Type_Armor)); iter != end(); ++iter)
    {
        if (iter->get<ESM::Armor>()->mBase->mData.mType != ESM::Armor::Shield)
            continue;
        if (iter->getClass().canBeEquipped(*iter, actor).first != 1)
            continue;
        std::pair<std::vector<int>, bool> shieldSlots =
            iter->getClass().getEquipmentSlots(*iter);
        int slot = shieldSlots.first[0];
        const ContainerStoreIterator& shield = slots_[slot];
        if (shield != end()
                && shield.getType() == Type_Armor && shield->get<ESM::Armor>()->mBase->mData.mType == ESM::Armor::Shield)
        {
            if (shield->getClass().getItemHealth(*shield) >= iter->getClass().getItemHealth(*iter))
                continue;
        }
        slots_[slot] = iter;
    }
}

void MWWorld::InventoryStore::autoEquip (const MWWorld::Ptr& actor)
{
    TSlots slots_;
    initSlots (slots_);

    // Disable model update during auto-equip
    mUpdatesEnabled = false;

    // Autoequip clothing, armor and weapons.
    // Equipping lights is handled in Actors::updateEquippedLight based on environment light.
    // Note: creatures ignore equipment armor rating and only equip shields
    // Use custom logic for them - select shield based on its health instead of armor rating
    autoEquipWeapon(actor, slots_);
    autoEquipArmor(actor, slots_);

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
    if (!mInventoryListener)
        return;

    mMagicEffects = MWMechanics::MagicEffects();

    if (actor.getClass().getCreatureStats(actor).isDead())
        return;

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
                params.resize(enchantment.mEffects.mList.size());

                int i=0;
                for (const ESM::ENAMstruct& effect : enchantment.mEffects.mList)
                {
                    int delta = effect.mMagnMax - effect.mMagnMin;
                    // Roll some dice, one for each effect
                    if (delta)
                        params[i].mRandom = Misc::Rng::rollDice(delta + 1) / static_cast<float>(delta);
                    // Try resisting each effect
                    params[i].mMultiplier = MWMechanics::getEffectMultiplier(effect.mEffectID, actor, actor);
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
            for (const ESM::ENAMstruct& effect : enchantment.mEffects.mList)
            {
                const ESM::MagicEffect *magicEffect =
                    MWBase::Environment::get().getWorld()->getStore().get<ESM::MagicEffect>().find (
                    effect.mEffectID);

                // Fully resisted or can't be applied to target?
                if (params[i].mMultiplier == 0 || !MWMechanics::checkEffectTarget(effect.mEffectID, actor, actor, actor == MWMechanics::getPlayer()))
                {
                    i++;
                    continue;
                }

                float magnitude = effect.mMagnMin + (effect.mMagnMax - effect.mMagnMin) * params[i].mRandom;
                magnitude *= params[i].mMultiplier;

                if (!existed)
                {
                    // During first auto equip, we don't play any sounds.
                    // Basically we don't want sounds when the actor is first loaded,
                    // the items should appear as if they'd always been equipped.
                    mInventoryListener->permanentEffectAdded(magicEffect, !mFirstAutoEquip);
                }

                if (magnitude)
                    mMagicEffects.add (effect, magnitude);

                i++;
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

bool MWWorld::InventoryStore::stacks(const ConstPtr& ptr1, const ConstPtr& ptr2) const
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

int MWWorld::InventoryStore::remove(const std::string& itemId, int count, const Ptr& actor)
{
    return remove(itemId, count, actor, false);
}

int MWWorld::InventoryStore::remove(const Ptr& item, int count, const Ptr& actor)
{
    return remove(item, count, actor, false);
}

int MWWorld::InventoryStore::remove(const std::string& itemId, int count, const Ptr& actor, bool equipReplacement)
{
    int toRemove = count;

    for (ContainerStoreIterator iter(begin()); iter != end() && toRemove > 0; ++iter)
        if (Misc::StringUtils::ciEqual(iter->getCellRef().getRefId(), itemId))
            toRemove -= remove(*iter, toRemove, actor, equipReplacement);

    flagAsModified();

    // number of removed items
    return count - toRemove;
}

int MWWorld::InventoryStore::remove(const Ptr& item, int count, const Ptr& actor, bool equipReplacement)
{
    int retCount = ContainerStore::remove(item, count, actor);

    bool wasEquipped = false;
    if (!item.getRefData().getCount())
    {
        for (int slot=0; slot < MWWorld::InventoryStore::Slots; ++slot)
        {
            if (mSlots[slot] == end())
                continue;

            if (*mSlots[slot] == item)
            {
                unequipSlot(slot, actor);
                wasEquipped = true;
                break;
            }
        }
    }

    // If an armor/clothing item is removed, try to find a replacement,
    // but not for the player nor werewolves, and not if the RemoveItem script command
    // was used (equipReplacement is false)
    if (equipReplacement && wasEquipped && (actor != MWMechanics::getPlayer())
            && actor.getClass().isNpc() && !actor.getClass().getNpcStats(actor).isWerewolf())
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

    if (mListener)
        mListener->itemRemoved(item, retCount);

    return retCount;
}

MWWorld::ContainerStoreIterator MWWorld::InventoryStore::unequipSlot(int slot, const MWWorld::Ptr& actor, bool fireEvent)
{
    if (slot<0 || slot>=static_cast<int> (mSlots.size()))
        throw std::runtime_error ("slot number out of range");

    ContainerStoreIterator it = mSlots[slot];

    if (it != end())
    {
        ContainerStoreIterator retval = it;

        // empty this slot
        mSlots[slot] = end();

        if (it->getRefData().getCount())
        {
            retval = restack(*it);

            if (actor == MWMechanics::getPlayer())
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

        if (fireEvent)
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

MWWorld::ContainerStoreIterator MWWorld::InventoryStore::unequipItemQuantity(const Ptr& item, const Ptr& actor, int count)
{
    if (!isEquipped(item))
        throw std::runtime_error ("attempt to unequip an item that is not currently equipped");
    if (count <= 0)
        throw std::runtime_error ("attempt to unequip nothing (count <= 0)");
    if (count > item.getRefData().getCount())
        throw std::runtime_error ("attempt to unequip more items than equipped");

    if (count == item.getRefData().getCount())
        return unequipItem(item, actor);

    // Move items to an existing stack if possible, otherwise split count items out into a new stack.
    // Moving counts manually here, since ContainerStore's restack can't target unequipped stacks.
    for (MWWorld::ContainerStoreIterator iter (begin()); iter != end(); ++iter)
    {
        if (stacks(*iter, item) && !isEquipped(*iter))
        {
            iter->getRefData().setCount(iter->getRefData().getCount() + count);
            item.getRefData().setCount(item.getRefData().getCount() - count);
            return iter;
        }
    }

    return unstack(item, actor, item.getRefData().getCount() - count);
}

MWWorld::InventoryStoreListener* MWWorld::InventoryStore::getInvListener()
{
    return mInventoryListener;
}

void MWWorld::InventoryStore::setInvListener(InventoryStoreListener *listener, const Ptr& actor)
{
    mInventoryListener = listener;
    updateMagicEffects(actor);
}

void MWWorld::InventoryStore::fireEquipmentChangedEvent(const Ptr& actor)
{
    if (!mUpdatesEnabled)
        return;
    if (mInventoryListener)
        mInventoryListener->equipmentChanged();

    // if player, update inventory window
    /*
    if (actor == MWMechanics::getPlayer())
    {
        MWBase::Environment::get().getWindowManager()->getInventoryWindow()->updateItemView();
    }
    */
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
        for (const ESM::ENAMstruct& effect : enchantment.mEffects.mList)
        {
            i++;
            // Don't get spell icon display information for enchantments that weren't actually applied
            if (mMagicEffects.get(MWMechanics::EffectKey(effect)).getMagnitude() == 0)
                continue;
            const EffectParams& params = mPermanentMagicEffectMagnitudes[(**iter).getCellRef().getRefId()][i-1];
            float magnitude = effect.mMagnMin + (effect.mMagnMax - effect.mMagnMin) * params.mRandom;
            magnitude *= params.mMultiplier;
            if (magnitude > 0)
                visitor.visit(MWMechanics::EffectKey(effect), (**iter).getClass().getName(**iter), (**iter).getCellRef().getRefId(), -1, magnitude);
        }
    }
}

void MWWorld::InventoryStore::purgeEffect(short effectId)
{
    for (TSlots::const_iterator it = mSlots.begin(); it != mSlots.end(); ++it)
    {
        if (*it != end())
            purgeEffect(effectId, (*it)->getCellRef().getRefId());
    }
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

        if ((*iter)->getCellRef().getRefId() != sourceId)
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

bool MWWorld::InventoryStore::isEquipped(const MWWorld::ConstPtr &item)
{
    for (int i=0; i < MWWorld::InventoryStore::Slots; ++i)
    {
        if (getSlot(i) != end() && *getSlot(i) == item)
            return true;
    }
    return false;
}

void MWWorld::InventoryStore::writeState(ESM::InventoryState &state) const
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
