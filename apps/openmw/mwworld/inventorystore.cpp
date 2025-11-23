#include "inventorystore.hpp"

#include <algorithm>
#include <iterator>

#include <components/esm3/inventorystate.hpp>

#include "../mwbase/environment.hpp"

#include "../mwmechanics/actorutil.hpp"
#include "../mwmechanics/npcstats.hpp"
#include "../mwmechanics/weapontype.hpp"

#include "class.hpp"
#include "esmstore.hpp"

void MWWorld::InventoryStore::copySlots(const InventoryStore& store)
{
    // some const-trickery, required because of a flaw in the handling of MW-references and the
    // resulting workarounds
    for (std::vector<ContainerStoreIterator>::const_iterator iter(const_cast<InventoryStore&>(store).mSlots.begin());
         iter != const_cast<InventoryStore&>(store).mSlots.end(); ++iter)
    {
        std::size_t distance = std::distance(const_cast<InventoryStore&>(store).begin(), *iter);

        ContainerStoreIterator slot = begin();

        std::advance(slot, distance);

        mSlots.push_back(slot);
    }

    // some const-trickery, required because of a flaw in the handling of MW-references and the
    // resulting workarounds
    std::size_t distance = std::distance(
        const_cast<InventoryStore&>(store).begin(), const_cast<InventoryStore&>(store).mSelectedEnchantItem);
    ContainerStoreIterator slot = begin();
    std::advance(slot, distance);
    mSelectedEnchantItem = slot;
}

void MWWorld::InventoryStore::initSlots(TSlots& slots)
{
    for (int i = 0; i < Slots; ++i)
        slots.push_back(end());
}

void MWWorld::InventoryStore::storeEquipmentState(
    const MWWorld::LiveCellRefBase& ref, size_t index, ESM::InventoryState& inventory) const
{
    for (int32_t i = 0; i < MWWorld::InventoryStore::Slots; ++i)
    {
        if (mSlots[i].getType() != -1 && mSlots[i]->getBase() == &ref)
            inventory.mEquipmentSlots[static_cast<uint32_t>(index)] = i;
    }

    if (mSelectedEnchantItem.getType() != -1 && mSelectedEnchantItem->getBase() == &ref)
        inventory.mSelectedEnchantItem = static_cast<uint32_t>(index);
}

void MWWorld::InventoryStore::readEquipmentState(
    const MWWorld::ContainerStoreIterator& iter, size_t index, const ESM::InventoryState& inventory)
{
    if (index == inventory.mSelectedEnchantItem)
        mSelectedEnchantItem = iter;

    auto found = inventory.mEquipmentSlots.find(static_cast<uint32_t>(index));
    if (found != inventory.mEquipmentSlots.end())
    {
        if (found->second < 0 || found->second >= MWWorld::InventoryStore::Slots)
            throw std::runtime_error("Invalid slot index in inventory state");

        // make sure the item can actually be equipped in this slot
        int32_t slot = found->second;
        std::pair<std::vector<int>, bool> allowedSlots = iter->getClass().getEquipmentSlots(*iter);
        if (!allowedSlots.first.size())
            return;
        if (std::find(allowedSlots.first.begin(), allowedSlots.first.end(), slot) == allowedSlots.first.end())
            slot = allowedSlots.first.front();

        // unstack if required
        if (!allowedSlots.second && iter->getCellRef().getCount() > 1)
        {
            int count = iter->getCellRef().getCount(false);
            MWWorld::ContainerStoreIterator newIter = addNewStack(*iter, count > 0 ? 1 : -1);
            iter->getCellRef().setCount(subtractItems(count, 1));
            mSlots[slot] = newIter;
        }
        else
            mSlots[slot] = iter;
    }
}

MWWorld::InventoryStore::InventoryStore()
    : ContainerStore()
    , mInventoryListener(nullptr)
    , mUpdatesEnabled(true)
    , mFirstAutoEquip(true)
    , mSelectedEnchantItem(end())
{
    initSlots(mSlots);
}

MWWorld::InventoryStore::InventoryStore(const InventoryStore& store)
    : ContainerStore(store)
    , mInventoryListener(store.mInventoryListener)
    , mUpdatesEnabled(store.mUpdatesEnabled)
    , mFirstAutoEquip(store.mFirstAutoEquip)
    , mSelectedEnchantItem(end())
{
    copySlots(store);
}

MWWorld::InventoryStore& MWWorld::InventoryStore::operator=(const InventoryStore& store)
{
    if (this == &store)
        return *this;

    mListener = store.mListener;
    mInventoryListener = store.mInventoryListener;
    mFirstAutoEquip = store.mFirstAutoEquip;
    mRechargingItemsUpToDate = false;
    ContainerStore::operator=(store);
    mSlots.clear();
    copySlots(store);
    return *this;
}

MWWorld::ContainerStoreIterator MWWorld::InventoryStore::add(
    const Ptr& itemPtr, int count, bool allowAutoEquip, bool resolve)
{
    const MWWorld::ContainerStoreIterator& retVal
        = MWWorld::ContainerStore::add(itemPtr, count, allowAutoEquip, resolve);

    // Auto-equip items if an armor/clothing item is added, but not for the player nor werewolves
    const Ptr& actor = getPtr();
    if (allowAutoEquip && actor != MWMechanics::getPlayer() && actor.getClass().isNpc()
        && !actor.getClass().getNpcStats(actor).isWerewolf())
    {
        auto type = itemPtr.getType();
        if (type == ESM::Armor::sRecordId || type == ESM::Clothing::sRecordId)
            autoEquip();
    }

    if (mListener)
        mListener->itemAdded(*retVal, count);

    return retVal;
}

void MWWorld::InventoryStore::equip(int slot, const ContainerStoreIterator& iterator)
{
    if (iterator == end())
        throw std::runtime_error("can't equip end() iterator, use unequip function instead");

    if (slot < 0 || slot >= static_cast<int>(mSlots.size()))
        throw std::runtime_error("slot number out of range");

    if (iterator.getContainerStore() != this)
        throw std::runtime_error("attempt to equip an item that is not in the inventory");

    std::pair<std::vector<int>, bool> slots;

    slots = iterator->getClass().getEquipmentSlots(*iterator);

    if (std::find(slots.first.begin(), slots.first.end(), slot) == slots.first.end())
        throw std::runtime_error("invalid slot");

    if (mSlots[slot] != end())
        unequipSlot(slot);

    // unstack item pointed to by iterator if required
    if (iterator != end() && !slots.second
        && iterator->getCellRef().getCount() > 1) // if slots.second is true, item can stay stacked when equipped
    {
        unstack(*iterator);
    }

    mSlots[slot] = iterator;

    flagAsModified();

    fireEquipmentChangedEvent();
}

void MWWorld::InventoryStore::unequipAll()
{
    mUpdatesEnabled = false;
    for (int slot = 0; slot < MWWorld::InventoryStore::Slots; ++slot)
        unequipSlot(slot);

    mUpdatesEnabled = true;

    fireEquipmentChangedEvent();
}

MWWorld::ContainerStoreIterator MWWorld::InventoryStore::getSlot(int slot)
{
    return findSlot(slot);
}

MWWorld::ConstContainerStoreIterator MWWorld::InventoryStore::getSlot(int slot) const
{
    return findSlot(slot);
}

MWWorld::ContainerStoreIterator MWWorld::InventoryStore::findSlot(int slot) const
{
    if (slot < 0 || slot >= static_cast<int>(mSlots.size()))
        throw std::runtime_error("slot number out of range");

    if (mSlots[slot] == end())
        return mSlots[slot];

    // NOTE: mSlots[slot]->getRefData().getCount() can be zero if the item is marked
    // for removal by a Lua script, but the removal action is not yet processed.
    // The item will be automatically unequiped in the current frame.

    return mSlots[slot];
}

void MWWorld::InventoryStore::autoEquipWeapon(TSlots& slots)
{
    const Ptr& actor = getPtr();
    if (!actor.getClass().isNpc())
    {
        // In original game creatures do not autoequip weapon, but we need it for weapon sheathing.
        // The only case when the difference is noticable - when this creature sells weapon.
        // So just disable weapon autoequipping for creatures which sells weapon.
        int services = actor.getClass().getServices(actor);
        bool sellsWeapon = services & (ESM::NPC::Weapon | ESM::NPC::MagicItems);
        if (sellsWeapon)
            return;
    }

    static const ESM::RefId weaponSkills[] = {
        ESM::Skill::LongBlade,
        ESM::Skill::Axe,
        ESM::Skill::Spear,
        ESM::Skill::ShortBlade,
        ESM::Skill::Marksman,
        ESM::Skill::BluntWeapon,
    };
    const size_t weaponSkillsLength = sizeof(weaponSkills) / sizeof(weaponSkills[0]);

    bool weaponSkillVisited[weaponSkillsLength] = { false };

    // give arrows/bolt with max damage by default
    int arrowMax = 0;
    int boltMax = 0;
    ContainerStoreIterator arrow(end());
    ContainerStoreIterator bolt(end());

    // rate ammo
    for (ContainerStoreIterator iter(begin(ContainerStore::Type_Weapon)); iter != end(); ++iter)
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
        float max = 0;
        int maxWeaponSkill = -1;

        for (int j = 0; j < static_cast<int>(weaponSkillsLength); ++j)
        {
            float skillValue = actor.getClass().getSkill(actor, weaponSkills[j]);
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

        for (ContainerStoreIterator iter(begin(ContainerStore::Type_Weapon)); iter != end(); ++iter)
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
            const MWWorld::LiveCellRef<ESM::Weapon>* ref = weapon->get<ESM::Weapon>();
            int type = ref->mBase->mData.mType;
            int ammotype = MWMechanics::getWeaponType(type)->mAmmoType;
            if (ammotype == ESM::Weapon::Arrow)
            {
                if (arrow == end())
                    hasAmmo = false;
                else
                    slots[Slot_Ammunition] = arrow;
            }
            else if (ammotype == ESM::Weapon::Bolt)
            {
                if (bolt == end())
                    hasAmmo = false;
                else
                    slots[Slot_Ammunition] = bolt;
            }

            if (hasAmmo)
            {
                std::pair<std::vector<int>, bool> itemsSlots = weapon->getClass().getEquipmentSlots(*weapon);

                if (!itemsSlots.first.empty())
                {
                    if (!itemsSlots.second)
                    {
                        if (weapon->getCellRef().getCount() > 1)
                        {
                            unstack(*weapon);
                        }
                    }

                    int slot = itemsSlots.first.front();
                    slots[slot] = weapon;

                    if (ammotype == ESM::Weapon::None)
                        slots[Slot_Ammunition] = end();
                }

                break;
            }
        }

        weaponSkillVisited[maxWeaponSkill] = true;
    }
}

void MWWorld::InventoryStore::autoEquipArmor(TSlots& slots)
{
    const Ptr& actor = getPtr();

    // Creatures only want shields and don't benefit from armor rating or unarmored skill
    const MWWorld::Class& actorCls = actor.getClass();
    const bool actorIsNpc = actorCls.isNpc();

    int equipmentTypes = ContainerStore::Type_Armor;
    float unarmoredRating = 0.f;
    if (actorIsNpc)
    {
        equipmentTypes |= ContainerStore::Type_Clothing;
        const auto& store = MWBase::Environment::get().getESMStore()->get<ESM::GameSetting>();
        const float fUnarmoredBase1 = store.find("fUnarmoredBase1")->mValue.getFloat();
        const float fUnarmoredBase2 = store.find("fUnarmoredBase2")->mValue.getFloat();
        const float unarmoredSkill = actorCls.getSkill(actor, ESM::Skill::Unarmored);
        unarmoredRating = (fUnarmoredBase1 * unarmoredSkill) * (fUnarmoredBase2 * unarmoredSkill);
        unarmoredRating = std::max(unarmoredRating, 0.f);
    }

    for (ContainerStoreIterator iter(begin(equipmentTypes)); iter != end(); ++iter)
    {
        Ptr test = *iter;
        const MWWorld::Class& testCls = test.getClass();
        const bool isArmor = iter.getType() == ContainerStore::Type_Armor;

        // Discard armor that is worse than unarmored for NPCs and non-shields for creatures
        if (isArmor)
        {
            if (actorIsNpc)
            {
                if (testCls.getSkillAdjustedArmorRating(test, actor) <= unarmoredRating)
                    continue;
            }
            else
            {
                if (test.get<ESM::Armor>()->mBase->mData.mType != ESM::Armor::Shield)
                    continue;
            }
        }

        // Don't equip the item if it cannot be equipped
        if (testCls.canBeEquipped(test, actor).first == 0)
            continue;

        const auto [itemSlots, canStack] = testCls.getEquipmentSlots(test);

        // checking if current item pointed by iter can be equipped
        for (const int slot : itemSlots)
        {
            // check if slot may require swapping if current item is more valuable
            if (slots.at(slot) != end())
            {
                Ptr old = *slots.at(slot);
                const MWWorld::Class& oldCls = old.getClass();
                unsigned int oldType = old.getType();

                if (!isArmor)
                {
                    // Armor should replace clothing and weapons, but clothing should only replace clothing
                    if (oldType != ESM::Clothing::sRecordId)
                        continue;

                    // If the left ring slot is filled, don't swap if the right ring is cheaper
                    if (slot == Slot_LeftRing)
                    {
                        if (slots.at(Slot_RightRing) == end())
                            continue;

                        Ptr rightRing = *slots.at(Slot_RightRing);
                        if (rightRing.getClass().getValue(rightRing) <= oldCls.getValue(old))
                            continue;
                    }

                    if (testCls.getValue(test) <= oldCls.getValue(old))
                        continue;
                }
                else if (oldType == ESM::Armor::sRecordId)
                {
                    const int32_t oldArmorType = old.get<ESM::Armor>()->mBase->mData.mType;
                    const int32_t newArmorType = test.get<ESM::Armor>()->mBase->mData.mType;
                    if (oldArmorType == newArmorType)
                    {
                        // For NPCs, compare armor rating; for creatures, compare condition
                        if (actorIsNpc)
                        {
                            const float rating = testCls.getSkillAdjustedArmorRating(test, actor);
                            const float oldRating = oldCls.getSkillAdjustedArmorRating(old, actor);
                            if (rating <= oldRating)
                                continue;
                        }
                        else
                        {
                            if (testCls.getItemHealth(test) <= oldCls.getItemHealth(old))
                                continue;
                        }
                    }
                    else if (oldArmorType < newArmorType)
                        continue;
                }
            }

            // unstack the item if required
            if (!canStack && test.getCellRef().getCount() > 1)
            {
                unstack(test);
            }

            // if we are here it means item can be equipped or swapped
            slots[slot] = iter;
            break;
        }
    }
}

void MWWorld::InventoryStore::autoEquip()
{
    TSlots slots;
    initSlots(slots);

    // Disable model update during auto-equip
    mUpdatesEnabled = false;

    // Autoequip clothing, armor and weapons.
    // Equipping lights is handled in Actors::updateEquippedLight based on environment light.
    autoEquipWeapon(slots);
    autoEquipArmor(slots);

    bool changed = false;

    for (std::size_t i = 0; i < slots.size(); ++i)
    {
        if (slots[i] != mSlots[i])
        {
            changed = true;
            break;
        }
    }
    mUpdatesEnabled = true;

    if (changed)
    {
        mSlots.swap(slots);
        fireEquipmentChangedEvent();
        flagAsModified();
    }
}

MWWorld::ContainerStoreIterator MWWorld::InventoryStore::getPreferredShield()
{
    TSlots slots;
    initSlots(slots);
    autoEquipArmor(slots);
    return slots[Slot_CarriedLeft];
}

bool MWWorld::InventoryStore::stacks(const ConstPtr& ptr1, const ConstPtr& ptr2) const
{
    bool canStack = MWWorld::ContainerStore::stacks(ptr1, ptr2);
    if (!canStack)
        return false;

    // don't stack if either item is currently equipped
    for (TSlots::const_iterator iter(mSlots.begin()); iter != mSlots.end(); ++iter)
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

int MWWorld::InventoryStore::remove(const Ptr& item, int count, bool equipReplacement, bool resolve)
{
    int retCount = ContainerStore::remove(item, count, equipReplacement, resolve);

    bool wasEquipped = false;
    if (!item.getCellRef().getCount())
    {
        for (int slot = 0; slot < MWWorld::InventoryStore::Slots; ++slot)
        {
            if (mSlots[slot] == end())
                continue;

            if (*mSlots[slot] == item)
            {
                unequipSlot(slot);
                wasEquipped = true;
                break;
            }
        }
    }

    // If an armor/clothing item is removed, try to find a replacement,
    // but not for the player nor werewolves, and not if the RemoveItem script command
    // was used (equipReplacement is false)
    const Ptr& actor = getPtr();
    if (equipReplacement && wasEquipped && (actor != MWMechanics::getPlayer()) && actor.getClass().isNpc()
        && !actor.getClass().getNpcStats(actor).isWerewolf())
    {
        auto type = item.getType();
        if (type == ESM::Armor::sRecordId || type == ESM::Clothing::sRecordId)
            autoEquip();
    }

    if (item.getCellRef().getCount() == 0 && mSelectedEnchantItem != end() && *mSelectedEnchantItem == item)
    {
        mSelectedEnchantItem = end();
    }

    if (mListener)
        mListener->itemRemoved(item, retCount);

    return retCount;
}

MWWorld::ContainerStoreIterator MWWorld::InventoryStore::unequipSlot(int slot, bool applyUpdates)
{
    if (slot < 0 || slot >= static_cast<int>(mSlots.size()))
        throw std::runtime_error("slot number out of range");

    ContainerStoreIterator it = mSlots[slot];

    if (it != end())
    {
        ContainerStoreIterator retval = it;

        // empty this slot
        mSlots[slot] = end();

        if (it->getCellRef().getCount())
        {
            retval = restack(*it);

            if (getPtr() == MWMechanics::getPlayer())
            {
                // Unset OnPCEquip Variable on item's script, if it has a script with that variable declared
                const ESM::RefId& script = it->getClass().getScript(*it);
                if (!script.empty())
                    (*it).getRefData().getLocals().setVarByInt(script, "onpcequip", 0);
            }

            if ((mSelectedEnchantItem != end()) && (mSelectedEnchantItem == it))
            {
                mSelectedEnchantItem = end();
            }
        }

        if (applyUpdates)
        {
            fireEquipmentChangedEvent();
        }

        return retval;
    }

    return it;
}

MWWorld::ContainerStoreIterator MWWorld::InventoryStore::unequipItem(const MWWorld::Ptr& item)
{
    for (int slot = 0; slot < MWWorld::InventoryStore::Slots; ++slot)
    {
        MWWorld::ContainerStoreIterator equipped = getSlot(slot);
        if (equipped != end() && *equipped == item)
            return unequipSlot(slot);
    }

    throw std::runtime_error("attempt to unequip an item that is not currently equipped");
}

MWWorld::ContainerStoreIterator MWWorld::InventoryStore::unequipItemQuantity(const Ptr& item, int count)
{
    if (!isEquipped(item))
        throw std::runtime_error("attempt to unequip an item that is not currently equipped");
    if (count <= 0)
        throw std::runtime_error("attempt to unequip nothing (count <= 0)");
    if (count > item.getCellRef().getCount())
        throw std::runtime_error("attempt to unequip more items than equipped");

    if (count == item.getCellRef().getCount())
        return unequipItem(item);

    // Move items to an existing stack if possible, otherwise split count items out into a new stack.
    // Moving counts manually here, since ContainerStore's restack can't target unequipped stacks.
    for (MWWorld::ContainerStoreIterator iter(begin()); iter != end(); ++iter)
    {
        if (stacks(*iter, item) && !isEquipped(*iter))
        {
            iter->getCellRef().setCount(addItems(iter->getCellRef().getCount(false), count));
            item.getCellRef().setCount(subtractItems(item.getCellRef().getCount(false), count));
            return iter;
        }
    }

    return unstack(item, item.getCellRef().getCount() - count);
}

MWWorld::InventoryStoreListener* MWWorld::InventoryStore::getInvListener() const
{
    return mInventoryListener;
}

void MWWorld::InventoryStore::setInvListener(InventoryStoreListener* listener)
{
    mInventoryListener = listener;
}

void MWWorld::InventoryStore::fireEquipmentChangedEvent()
{
    if (!mUpdatesEnabled)
        return;
    if (mInventoryListener)
        mInventoryListener->equipmentChanged();

    // if player, update inventory window
    /*
    if (mActor == MWMechanics::getPlayer())
    {
        MWBase::Environment::get().getWindowManager()->getInventoryWindow()->updateItemView();
    }
    */
}

void MWWorld::InventoryStore::clear()
{
    mSlots.clear();
    initSlots(mSlots);
    ContainerStore::clear();
}

bool MWWorld::InventoryStore::isEquipped(const MWWorld::ConstPtr& item)
{
    for (int i = 0; i < MWWorld::InventoryStore::Slots; ++i)
    {
        if (getSlot(i) != end() && *getSlot(i) == item)
            return true;
    }
    return false;
}

bool MWWorld::InventoryStore::isEquipped(const ESM::RefId& id)
{
    for (int i = 0; i < MWWorld::InventoryStore::Slots; ++i)
    {
        if (getSlot(i) != end() && getSlot(i)->getCellRef().getRefId() == id)
            return true;
    }
    return false;
}

bool MWWorld::InventoryStore::isFirstEquip()
{
    bool first = mFirstAutoEquip;
    mFirstAutoEquip = false;
    return first;
}
