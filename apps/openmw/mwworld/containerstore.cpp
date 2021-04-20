#include "containerstore.hpp"

#include <cassert>
#include <typeinfo>
#include <stdexcept>

#include <components/debug/debuglog.hpp>
#include <components/esm/inventorystate.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"

#include "../mwmechanics/creaturestats.hpp"
#include "../mwmechanics/levelledlist.hpp"
#include "../mwmechanics/actorutil.hpp"
#include "../mwmechanics/recharge.hpp"

#include "manualref.hpp"
#include "refdata.hpp"
#include "class.hpp"
#include "localscripts.hpp"
#include "player.hpp"

namespace
{
    void addScripts(MWWorld::ContainerStore& store, MWWorld::CellStore* cell)
    {
        auto& scripts = MWBase::Environment::get().getWorld()->getLocalScripts();
        for(const auto&& ptr : store)
        {
            const std::string& script = ptr.getClass().getScript(ptr);
            if(!script.empty())
            {
                MWWorld::Ptr item = ptr;
                item.mCell = cell;
                scripts.add(script, item);
            }
        }
    }

    template<typename T>
    float getTotalWeight (const MWWorld::CellRefList<T>& cellRefList)
    {
        float sum = 0;

        for (const auto& iter : cellRefList.mList)
        {
            if (iter.mData.getCount()>0)
                sum += iter.mData.getCount()*iter.mBase->mData.mWeight;
        }

        return sum;
    }

    template<typename T>
    MWWorld::Ptr searchId (MWWorld::CellRefList<T>& list, const std::string& id,
        MWWorld::ContainerStore *store)
    {
        store->resolve();
        std::string id2 = Misc::StringUtils::lowerCase (id);

        for (auto& iter : list.mList)
        {
            if (Misc::StringUtils::ciEqual(iter.mBase->mId, id2) && iter.mData.getCount())
            {
                MWWorld::Ptr ptr (&iter, nullptr);
                ptr.setContainerStore (store);
                return ptr;
            }
        }

        return MWWorld::Ptr();
    }
}

MWWorld::ResolutionListener::~ResolutionListener()
{
    try
    {
        mStore.unresolve();
    }
    catch(const std::exception& e)
    {
        Log(Debug::Error) << "Failed to clear temporary container contents: " << e.what();
    }
}

template<typename T>
MWWorld::ContainerStoreIterator MWWorld::ContainerStore::getState (CellRefList<T>& collection,
    const ESM::ObjectState& state)
{
    if (!LiveCellRef<T>::checkState (state))
        return ContainerStoreIterator (this); // not valid anymore with current content files -> skip

    const T *record = MWBase::Environment::get().getWorld()->getStore().
        get<T>().search (state.mRef.mRefID);

    if (!record)
        return ContainerStoreIterator (this);

    LiveCellRef<T> ref (record);
    ref.load (state);
    collection.mList.push_back (ref);

    return ContainerStoreIterator (this, --collection.mList.end());
}

void MWWorld::ContainerStore::storeEquipmentState(const MWWorld::LiveCellRefBase &ref, int index, ESM::InventoryState &inventory) const
{
}

void MWWorld::ContainerStore::readEquipmentState(const MWWorld::ContainerStoreIterator& iter, int index, const ESM::InventoryState &inventory)
{
}

template<typename T>
void MWWorld::ContainerStore::storeState (const LiveCellRef<T>& ref, ESM::ObjectState& state) const
{
    ref.save (state);
}

template<typename T>
void MWWorld::ContainerStore::storeStates (const CellRefList<T>& collection,
    ESM::InventoryState& inventory, int& index, bool equipable) const
{
    for (const auto& iter : collection.mList)
    {
        if (iter.mData.getCount() == 0)
            continue;
        ESM::ObjectState state;
        storeState (iter, state);
        if (equipable)
            storeEquipmentState(iter, index, inventory);
        inventory.mItems.push_back (state);
        ++index;
    }
}

const std::string MWWorld::ContainerStore::sGoldId = "gold_001";

MWWorld::ContainerStore::ContainerStore()
    : mListener(nullptr)
    , mRechargingItemsUpToDate(false)
    , mCachedWeight (0)
    , mWeightUpToDate (false)
    , mModified(false)
    , mResolved(false)
    , mSeed()
    , mPtr() {}

MWWorld::ContainerStore::~ContainerStore() {}

MWWorld::ConstContainerStoreIterator MWWorld::ContainerStore::cbegin (int mask) const
{
    return ConstContainerStoreIterator (mask, this);
}

MWWorld::ConstContainerStoreIterator MWWorld::ContainerStore::cend() const
{
    return ConstContainerStoreIterator (this);
}

MWWorld::ConstContainerStoreIterator MWWorld::ContainerStore::begin (int mask) const
{
    return cbegin(mask);
}

MWWorld::ConstContainerStoreIterator MWWorld::ContainerStore::end() const
{
    return cend();
}

MWWorld::ContainerStoreIterator MWWorld::ContainerStore::begin (int mask)
{
    return ContainerStoreIterator (mask, this);
}

MWWorld::ContainerStoreIterator MWWorld::ContainerStore::end()
{
    return ContainerStoreIterator (this);
}

int MWWorld::ContainerStore::count(const std::string &id) const
{
    int total=0;
    for (const auto&& iter : *this)
        if (Misc::StringUtils::ciEqual(iter.getCellRef().getRefId(), id))
            total += iter.getRefData().getCount();
    return total;
}

MWWorld::ContainerStoreListener* MWWorld::ContainerStore::getContListener() const
{
    return mListener;
}


void MWWorld::ContainerStore::setContListener(MWWorld::ContainerStoreListener* listener)
{
    mListener = listener;
}

MWWorld::ContainerStoreIterator MWWorld::ContainerStore::unstack(const Ptr &ptr, const Ptr& container, int count)
{
    resolve();
    if (ptr.getRefData().getCount() <= count)
        return end();
    MWWorld::ContainerStoreIterator it = addNewStack(ptr, subtractItems(ptr.getRefData().getCount(false), count));
    const std::string script = it->getClass().getScript(*it);
    if (!script.empty())
        MWBase::Environment::get().getWorld()->getLocalScripts().add(script, *it);

    remove(ptr, ptr.getRefData().getCount()-count, container);

    return it;
}

MWWorld::ContainerStoreIterator MWWorld::ContainerStore::restack(const MWWorld::Ptr& item)
{
    resolve();
    MWWorld::ContainerStoreIterator retval = end();
    for (MWWorld::ContainerStoreIterator iter (begin()); iter != end(); ++iter)
    {
        if (item == *iter)
        {
            retval = iter;
            break;
        }
    }

    if (retval == end())
        throw std::runtime_error("item is not from this container");

    for (MWWorld::ContainerStoreIterator iter (begin()); iter != end(); ++iter)
    {
        if (stacks(*iter, item))
        {
            iter->getRefData().setCount(addItems(iter->getRefData().getCount(false), item.getRefData().getCount(false)));
            item.getRefData().setCount(0);
            retval = iter;
            break;
        }
    }
    return retval;
}

bool MWWorld::ContainerStore::stacks(const ConstPtr& ptr1, const ConstPtr& ptr2) const
{
    const MWWorld::Class& cls1 = ptr1.getClass();
    const MWWorld::Class& cls2 = ptr2.getClass();

    if (!Misc::StringUtils::ciEqual(ptr1.getCellRef().getRefId(), ptr2.getCellRef().getRefId()))
        return false;

    // If it has an enchantment, don't stack when some of the charge is already used
    if (!ptr1.getClass().getEnchantment(ptr1).empty())
    {
        const ESM::Enchantment* enchantment = MWBase::Environment::get().getWorld()->getStore().get<ESM::Enchantment>().find(
                    ptr1.getClass().getEnchantment(ptr1));
        float maxCharge = static_cast<float>(enchantment->mData.mCharge);
        float enchantCharge1 = ptr1.getCellRef().getEnchantmentCharge() == -1 ? maxCharge : ptr1.getCellRef().getEnchantmentCharge();
        float enchantCharge2 = ptr2.getCellRef().getEnchantmentCharge() == -1 ? maxCharge : ptr2.getCellRef().getEnchantmentCharge();
        if (enchantCharge1 != maxCharge || enchantCharge2 != maxCharge)
            return false;
    }

    return ptr1 != ptr2 // an item never stacks onto itself
        && ptr1.getCellRef().getSoul() == ptr2.getCellRef().getSoul()

        && ptr1.getClass().getRemainingUsageTime(ptr1) == ptr2.getClass().getRemainingUsageTime(ptr2)

        // Items with scripts never stack
        && cls1.getScript(ptr1).empty()
        && cls2.getScript(ptr2).empty()

        // item that is already partly used up never stacks
        && (!cls1.hasItemHealth(ptr1) || (
                cls1.getItemHealth(ptr1) == cls1.getItemMaxHealth(ptr1)
            && cls2.getItemHealth(ptr2) == cls2.getItemMaxHealth(ptr2)));
}

MWWorld::ContainerStoreIterator MWWorld::ContainerStore::add(const std::string &id, int count, const Ptr &actorPtr)
{
    MWWorld::ManualRef ref(MWBase::Environment::get().getWorld()->getStore(), id, count);
    return add(ref.getPtr(), count, actorPtr);
}

MWWorld::ContainerStoreIterator MWWorld::ContainerStore::add (const Ptr& itemPtr, int count, const Ptr& actorPtr, bool /*allowAutoEquip*/, bool resolve)
{
    Ptr player = MWBase::Environment::get().getWorld ()->getPlayerPtr();

    MWWorld::ContainerStoreIterator it = addImp(itemPtr, count, resolve);

    // The copy of the original item we just made
    MWWorld::Ptr item = *it;

    // we may have copied an item from the world, so reset a few things first
    item.getRefData().setBaseNode(nullptr); // Especially important, otherwise scripts on the item could think that it's actually in a cell
    ESM::Position pos;
    pos.rot[0] = 0;
    pos.rot[1] = 0;
    pos.rot[2] = 0;
    pos.pos[0] = 0;
    pos.pos[1] = 0;
    pos.pos[2] = 0;
    item.getCellRef().setPosition(pos);

    // We do not need to store owners for items in container stores - we do not use it anyway.
    item.getCellRef().setOwner("");
    item.getCellRef().resetGlobalVariable();
    item.getCellRef().setFaction("");
    item.getCellRef().setFactionRank(-2);

    // must reset the RefNum on the copied item, so that the RefNum on the original item stays unique
    // maybe we should do this in the copy constructor instead?
    item.getCellRef().unsetRefNum(); // destroy link to content file

    std::string script = item.getClass().getScript(item);
    if (!script.empty())
    {
        if (actorPtr == player)
        {
            // Items in player's inventory have cell set to 0, so their scripts will never be removed
            item.mCell = nullptr;
        }
        else
        {
            // Set mCell to the cell of the container/actor, so that the scripts are removed properly when
            // the cell of the container/actor goes inactive
            item.mCell = actorPtr.getCell();
        }

        item.mContainerStore = this;

        MWBase::Environment::get().getWorld()->getLocalScripts().add(script, item);

        // Set OnPCAdd special variable, if it is declared
        // Make sure to do this *after* we have added the script to LocalScripts
        if (actorPtr == player)
            item.getRefData().getLocals().setVarByInt(script, "onpcadd", 1);
    }

    // we should not fire event for InventoryStore yet - it has some custom logic
    if (mListener && !actorPtr.getClass().hasInventoryStore(actorPtr))
        mListener->itemAdded(item, count);

    return it;
}

MWWorld::ContainerStoreIterator MWWorld::ContainerStore::addImp (const Ptr& ptr, int count, bool markModified)
{
    if(markModified)
        resolve();
    int type = getType(ptr);

    const MWWorld::ESMStore &esmStore =
        MWBase::Environment::get().getWorld()->getStore();

    // gold needs special handling: when it is inserted into a container, the base object automatically becomes Gold_001
    // this ensures that gold piles of different sizes stack with each other (also, several scripts rely on Gold_001 for detecting player gold)
    if(ptr.getClass().isGold(ptr))
    {
        int realCount = count * ptr.getClass().getValue(ptr);

        for (MWWorld::ContainerStoreIterator iter (begin(type)); iter!=end(); ++iter)
        {
            if (Misc::StringUtils::ciEqual((*iter).getCellRef().getRefId(), MWWorld::ContainerStore::sGoldId))
            {
                iter->getRefData().setCount(addItems(iter->getRefData().getCount(false), realCount));
                flagAsModified();
                return iter;
            }
        }

        MWWorld::ManualRef ref(esmStore, MWWorld::ContainerStore::sGoldId, realCount);
        return addNewStack(ref.getPtr(), realCount);
    }

    // determine whether to stack or not
    for (MWWorld::ContainerStoreIterator iter (begin(type)); iter!=end(); ++iter)
    {
        if (stacks(*iter, ptr))
        {
            // stack
            iter->getRefData().setCount(addItems(iter->getRefData().getCount(false), count));

            flagAsModified();
            return iter;
        }
    }
    // if we got here, this means no stacking
    return addNewStack(ptr, count);
}

MWWorld::ContainerStoreIterator MWWorld::ContainerStore::addNewStack (const ConstPtr& ptr, int count)
{
    ContainerStoreIterator it = begin();

    switch (getType(ptr))
    {
        case Type_Potion: potions.mList.push_back (*ptr.get<ESM::Potion>()); it = ContainerStoreIterator(this, --potions.mList.end()); break;
        case Type_Apparatus: appas.mList.push_back (*ptr.get<ESM::Apparatus>()); it = ContainerStoreIterator(this, --appas.mList.end()); break;
        case Type_Armor: armors.mList.push_back (*ptr.get<ESM::Armor>()); it = ContainerStoreIterator(this, --armors.mList.end()); break;
        case Type_Book: books.mList.push_back (*ptr.get<ESM::Book>()); it = ContainerStoreIterator(this, --books.mList.end()); break;
        case Type_Clothing: clothes.mList.push_back (*ptr.get<ESM::Clothing>()); it = ContainerStoreIterator(this, --clothes.mList.end()); break;
        case Type_Ingredient: ingreds.mList.push_back (*ptr.get<ESM::Ingredient>()); it = ContainerStoreIterator(this, --ingreds.mList.end()); break;
        case Type_Light: lights.mList.push_back (*ptr.get<ESM::Light>()); it = ContainerStoreIterator(this, --lights.mList.end()); break;
        case Type_Lockpick: lockpicks.mList.push_back (*ptr.get<ESM::Lockpick>()); it = ContainerStoreIterator(this, --lockpicks.mList.end()); break;
        case Type_Miscellaneous: miscItems.mList.push_back (*ptr.get<ESM::Miscellaneous>()); it = ContainerStoreIterator(this, --miscItems.mList.end()); break;
        case Type_Probe: probes.mList.push_back (*ptr.get<ESM::Probe>()); it = ContainerStoreIterator(this, --probes.mList.end()); break;
        case Type_Repair: repairs.mList.push_back (*ptr.get<ESM::Repair>()); it = ContainerStoreIterator(this, --repairs.mList.end()); break;
        case Type_Weapon: weapons.mList.push_back (*ptr.get<ESM::Weapon>()); it = ContainerStoreIterator(this, --weapons.mList.end()); break;
    }

    it->getRefData().setCount(count);

    flagAsModified();
    return it;
}

void MWWorld::ContainerStore::rechargeItems(float duration)
{
    if (!mRechargingItemsUpToDate)
    {
        updateRechargingItems();
        mRechargingItemsUpToDate = true;
    }
    for (auto& it : mRechargingItems)
    {
        if (!MWMechanics::rechargeItem(*it.first, it.second, duration))
            continue;

        // attempt to restack when fully recharged
        if (it.first->getCellRef().getEnchantmentCharge() == it.second)
            it.first = restack(*it.first);
    }
}

void MWWorld::ContainerStore::updateRechargingItems()
{
    mRechargingItems.clear();
    for (ContainerStoreIterator it = begin(); it != end(); ++it)
    {
        const std::string& enchantmentId = it->getClass().getEnchantment(*it);
        if (!enchantmentId.empty())
        {
            const ESM::Enchantment* enchantment = MWBase::Environment::get().getWorld()->getStore().get<ESM::Enchantment>().search(enchantmentId);
            if (!enchantment)
            {
                Log(Debug::Warning) << "Warning: Can't find enchantment '" << enchantmentId << "' on item " << it->getCellRef().getRefId();
                continue;
            }

            if (enchantment->mData.mType == ESM::Enchantment::WhenUsed
                    || enchantment->mData.mType == ESM::Enchantment::WhenStrikes)
                mRechargingItems.emplace_back(it, static_cast<float>(enchantment->mData.mCharge));
        }
    }
}

int MWWorld::ContainerStore::remove(const std::string& itemId, int count, const Ptr& actor, bool equipReplacement, bool resolveFirst)
{
    if(resolveFirst)
        resolve();
    int toRemove = count;

    for (ContainerStoreIterator iter(begin()); iter != end() && toRemove > 0; ++iter)
        if (Misc::StringUtils::ciEqual(iter->getCellRef().getRefId(), itemId))
            toRemove -= remove(*iter, toRemove, actor, equipReplacement, resolveFirst);

    flagAsModified();

    // number of removed items
    return count - toRemove;
}

bool MWWorld::ContainerStore::hasVisibleItems() const
{
    for (const auto&& iter : *this)
    {
        if (iter.getClass().showsInInventory(iter))
            return true;
    }

    return false;
}

int MWWorld::ContainerStore::remove(const Ptr& item, int count, const Ptr& actor, bool equipReplacement, bool resolveFirst)
{
    assert(this == item.getContainerStore());
    if(resolveFirst)
        resolve();

    int toRemove = count;
    RefData& itemRef = item.getRefData();

    if (itemRef.getCount() <= toRemove)
    {
        toRemove -= itemRef.getCount();
        itemRef.setCount(0);
    }
    else
    {
        itemRef.setCount(subtractItems(itemRef.getCount(false), toRemove));
        toRemove = 0;
    }

    flagAsModified();

    // we should not fire event for InventoryStore yet - it has some custom logic
    if (mListener && !actor.getClass().hasInventoryStore(actor))
        mListener->itemRemoved(item, count - toRemove);

    // number of removed items
    return count - toRemove;
}

void MWWorld::ContainerStore::fill (const ESM::InventoryList& items, const std::string& owner, Misc::Rng::Seed& seed)
{
    for (const ESM::ContItem& iter : items.mList)
    {
        std::string id = Misc::StringUtils::lowerCase(iter.mItem);
        addInitialItem(id, owner, iter.mCount, &seed);
    }

    flagAsModified();
    mResolved = true;
}

void MWWorld::ContainerStore::fillNonRandom (const ESM::InventoryList& items, const std::string& owner, unsigned int seed)
{
    mSeed = seed;
    for (const ESM::ContItem& iter : items.mList)
    {
        std::string id = Misc::StringUtils::lowerCase(iter.mItem);
        addInitialItem(id, owner, iter.mCount, nullptr);
    }

    flagAsModified();
    mResolved = false;
}

void MWWorld::ContainerStore::addInitialItem (const std::string& id, const std::string& owner, int count,
                                            Misc::Rng::Seed* seed, bool topLevel)
{
    if (count == 0) return; //Don't restock with nothing.
    try
    {
        ManualRef ref (MWBase::Environment::get().getWorld()->getStore(), id, count);
        if (ref.getPtr().getClass().getScript(ref.getPtr()).empty())
        {
            addInitialItemImp(ref.getPtr(), owner, count, seed, topLevel);
        }
        else
        {
            // Adding just one item per time to make sure there isn't a stack of scripted items
            for (int i = 0; i < std::abs(count); i++)
                addInitialItemImp(ref.getPtr(), owner, count < 0 ? -1 : 1, seed, topLevel);
        }
    }
    catch (const std::exception& e)
    {
        Log(Debug::Warning) << "Warning: MWWorld::ContainerStore::addInitialItem: " << e.what();
    }
}

void MWWorld::ContainerStore::addInitialItemImp(const MWWorld::Ptr& ptr, const std::string& owner, int count,
                                               Misc::Rng::Seed* seed, bool topLevel)
{
    if (ptr.getTypeName()==typeid (ESM::ItemLevList).name())
    {
        if(!seed)
            return;
        const ESM::ItemLevList* levItemList = ptr.get<ESM::ItemLevList>()->mBase;

        if (topLevel && std::abs(count) > 1 && levItemList->mFlags & ESM::ItemLevList::Each)
        {
            for (int i=0; i<std::abs(count); ++i)
                addInitialItem(ptr.getCellRef().getRefId(), owner, count > 0 ? 1 : -1, seed, true);
            return;
        }
        else
        {
            std::string itemId = MWMechanics::getLevelledItem(ptr.get<ESM::ItemLevList>()->mBase, false, *seed);
            if (itemId.empty())
                return;
            addInitialItem(itemId, owner, count, seed, false);
        }
    }
    else
    {
        ptr.getCellRef().setOwner(owner);
        addImp (ptr, count, false);
    }
}

void MWWorld::ContainerStore::clear()
{
    for (auto&& iter : *this)
        iter.getRefData().setCount (0);

    flagAsModified();
    mModified = true;
}

void MWWorld::ContainerStore::flagAsModified()
{
    mWeightUpToDate = false;
    mRechargingItemsUpToDate = false;
}

bool MWWorld::ContainerStore::isResolved() const
{
    return mResolved;
}

void MWWorld::ContainerStore::resolve()
{
    if(!mResolved && !mPtr.isEmpty())
    {
        for(const auto&& ptr : *this)
            ptr.getRefData().setCount(0);
        Misc::Rng::Seed seed{mSeed};
        fill(mPtr.get<ESM::Container>()->mBase->mInventory, "", seed);
        addScripts(*this, mPtr.mCell);
    }
    mModified = true;
}

MWWorld::ResolutionHandle MWWorld::ContainerStore::resolveTemporarily()
{
    if(mModified)
        return {};
    std::shared_ptr<ResolutionListener> listener = mResolutionListener.lock();
    if(!listener)
    {
        listener = std::make_shared<ResolutionListener>(*this);
        mResolutionListener = listener;
    }
    if(!mResolved && !mPtr.isEmpty())
    {
        for(const auto&& ptr : *this)
            ptr.getRefData().setCount(0);
        Misc::Rng::Seed seed{mSeed};
        fill(mPtr.get<ESM::Container>()->mBase->mInventory, "", seed);
        addScripts(*this, mPtr.mCell);
    }
    return {listener};
}

void MWWorld::ContainerStore::unresolve()
{
    if (mModified)
        return;

    if (mResolved && !mPtr.isEmpty())
    {
        for(const auto&& ptr : *this)
            ptr.getRefData().setCount(0);
        fillNonRandom(mPtr.get<ESM::Container>()->mBase->mInventory, "", mSeed);
        addScripts(*this, mPtr.mCell);
        mResolved = false;
    }
}

float MWWorld::ContainerStore::getWeight() const
{
    if (!mWeightUpToDate)
    {
        mCachedWeight = 0;

        mCachedWeight += getTotalWeight (potions);
        mCachedWeight += getTotalWeight (appas);
        mCachedWeight += getTotalWeight (armors);
        mCachedWeight += getTotalWeight (books);
        mCachedWeight += getTotalWeight (clothes);
        mCachedWeight += getTotalWeight (ingreds);
        mCachedWeight += getTotalWeight (lights);
        mCachedWeight += getTotalWeight (lockpicks);
        mCachedWeight += getTotalWeight (miscItems);
        mCachedWeight += getTotalWeight (probes);
        mCachedWeight += getTotalWeight (repairs);
        mCachedWeight += getTotalWeight (weapons);

        mWeightUpToDate = true;
    }

    return mCachedWeight;
}

int MWWorld::ContainerStore::getType (const ConstPtr& ptr)
{
    if (ptr.isEmpty())
        throw std::runtime_error ("can't put a non-existent object into a container");

    if (ptr.getTypeName()==typeid (ESM::Potion).name())
        return Type_Potion;

    if (ptr.getTypeName()==typeid (ESM::Apparatus).name())
        return Type_Apparatus;

    if (ptr.getTypeName()==typeid (ESM::Armor).name())
        return Type_Armor;

    if (ptr.getTypeName()==typeid (ESM::Book).name())
        return Type_Book;

    if (ptr.getTypeName()==typeid (ESM::Clothing).name())
        return Type_Clothing;

    if (ptr.getTypeName()==typeid (ESM::Ingredient).name())
        return Type_Ingredient;

    if (ptr.getTypeName()==typeid (ESM::Light).name())
        return Type_Light;

    if (ptr.getTypeName()==typeid (ESM::Lockpick).name())
        return Type_Lockpick;

    if (ptr.getTypeName()==typeid (ESM::Miscellaneous).name())
        return Type_Miscellaneous;

    if (ptr.getTypeName()==typeid (ESM::Probe).name())
        return Type_Probe;

    if (ptr.getTypeName()==typeid (ESM::Repair).name())
        return Type_Repair;

    if (ptr.getTypeName()==typeid (ESM::Weapon).name())
        return Type_Weapon;

    throw std::runtime_error (
        "Object '" + ptr.getCellRef().getRefId() + "' of type " + ptr.getTypeName() + " can not be placed into a container");
}

MWWorld::Ptr MWWorld::ContainerStore::findReplacement(const std::string& id)
{
    MWWorld::Ptr item;
    int itemHealth = 1;
    for (auto&& iter : *this)
    {
        int iterHealth = iter.getClass().hasItemHealth(iter) ? iter.getClass().getItemHealth(iter) : 1;
        if (Misc::StringUtils::ciEqual(iter.getCellRef().getRefId(), id))
        {
            // Prefer the stack with the lowest remaining uses
            // Try to get item with zero durability only if there are no other items found
            if (item.isEmpty() ||
                (iterHealth > 0 && iterHealth < itemHealth) ||
                (itemHealth <= 0 && iterHealth > 0))
            {
                item = iter;
                itemHealth = iterHealth;
            }
        }
    }

    return item;
}

MWWorld::Ptr MWWorld::ContainerStore::search (const std::string& id)
{
    resolve();
    {
        Ptr ptr = searchId (potions, id, this);
        if (!ptr.isEmpty())
            return ptr;
    }

    {
        Ptr ptr = searchId (appas, id, this);
        if (!ptr.isEmpty())
            return ptr;
    }

    {
        Ptr ptr = searchId (armors, id, this);
        if (!ptr.isEmpty())
            return ptr;
    }

    {
        Ptr ptr = searchId (books, id, this);
        if (!ptr.isEmpty())
            return ptr;
    }

    {
        Ptr ptr = searchId (clothes, id, this);
        if (!ptr.isEmpty())
            return ptr;
    }

    {
        Ptr ptr = searchId (ingreds, id, this);
        if (!ptr.isEmpty())
            return ptr;
    }

    {
        Ptr ptr = searchId (lights, id, this);
        if (!ptr.isEmpty())
            return ptr;
    }

    {
        Ptr ptr = searchId (lockpicks, id, this);
        if (!ptr.isEmpty())
            return ptr;
    }

    {
        Ptr ptr = searchId (miscItems, id, this);
        if (!ptr.isEmpty())
            return ptr;
    }

    {
        Ptr ptr = searchId (probes, id, this);
        if (!ptr.isEmpty())
            return ptr;
    }

    {
        Ptr ptr = searchId (repairs, id, this);
        if (!ptr.isEmpty())
            return ptr;
    }

    {
        Ptr ptr = searchId (weapons, id, this);
        if (!ptr.isEmpty())
            return ptr;
    }

    return Ptr();
}

int MWWorld::ContainerStore::addItems(int count1, int count2)
{
    int sum = std::abs(count1) + std::abs(count2);
    if(count1 < 0 || count2 < 0)
        return -sum;
    return sum;
}

int MWWorld::ContainerStore::subtractItems(int count1, int count2)
{
    int sum = std::abs(count1) - std::abs(count2);
    if(count1 < 0 || count2 < 0)
        return -sum;
    return sum;
}

void MWWorld::ContainerStore::writeState (ESM::InventoryState& state) const
{
    state.mItems.clear();

    int index = 0;
    storeStates (potions, state, index);
    storeStates (appas, state, index);
    storeStates (armors, state, index, true);
    storeStates (books, state, index, true); // not equipable as such, but for selectedEnchantItem
    storeStates (clothes, state, index, true);
    storeStates (ingreds, state, index);
    storeStates (lockpicks, state, index, true);
    storeStates (miscItems, state, index);
    storeStates (probes, state, index, true);
    storeStates (repairs, state, index);
    storeStates (weapons, state, index, true);
    storeStates (lights, state, index, true);
}

void MWWorld::ContainerStore::readState (const ESM::InventoryState& inventory)
{
    clear();
    mModified = true;
    mResolved = true;

    int index = 0;
    for (const ESM::ObjectState& state : inventory.mItems)
    {
        int type = MWBase::Environment::get().getWorld()->getStore().find(state.mRef.mRefID);

        int thisIndex = index++;

        switch (type)
        {
            case ESM::REC_ALCH: getState (potions, state); break;
            case ESM::REC_APPA: getState (appas, state); break;
            case ESM::REC_ARMO: readEquipmentState (getState (armors, state), thisIndex, inventory); break;
            case ESM::REC_BOOK: readEquipmentState (getState (books, state), thisIndex, inventory); break; // not equipable as such, but for selectedEnchantItem
            case ESM::REC_CLOT: readEquipmentState (getState (clothes, state), thisIndex, inventory); break;
            case ESM::REC_INGR: getState (ingreds, state); break;
            case ESM::REC_LOCK: readEquipmentState (getState (lockpicks, state), thisIndex, inventory); break;
            case ESM::REC_MISC: getState (miscItems, state); break;
            case ESM::REC_PROB: readEquipmentState (getState (probes, state), thisIndex, inventory); break;
            case ESM::REC_REPA: getState (repairs, state); break;
            case ESM::REC_WEAP: readEquipmentState (getState (weapons, state), thisIndex, inventory); break;
            case ESM::REC_LIGH: readEquipmentState (getState (lights, state), thisIndex, inventory); break;
            case 0:
                Log(Debug::Warning) << "Dropping inventory reference to '" << state.mRef.mRefID << "' (object no longer exists)";
                break;
            default:
                Log(Debug::Warning) << "Warning: Invalid item type in inventory state, refid " << state.mRef.mRefID;
                break;
        }
    }
}

template<class PtrType>
template<class T>
void MWWorld::ContainerStoreIteratorBase<PtrType>::copy (const ContainerStoreIteratorBase<T>& src)
{
    mType = src.mType;
    mMask = src.mMask;
    mContainer = src.mContainer;
    mPtr = src.mPtr;

    switch (src.mType)
    {
        case MWWorld::ContainerStore::Type_Potion: mPotion = src.mPotion; break;
        case MWWorld::ContainerStore::Type_Apparatus: mApparatus = src.mApparatus; break;
        case MWWorld::ContainerStore::Type_Armor: mArmor = src.mArmor; break;
        case MWWorld::ContainerStore::Type_Book: mBook = src.mBook; break;
        case MWWorld::ContainerStore::Type_Clothing: mClothing = src.mClothing; break;
        case MWWorld::ContainerStore::Type_Ingredient: mIngredient = src.mIngredient; break;
        case MWWorld::ContainerStore::Type_Light: mLight = src.mLight; break;
        case MWWorld::ContainerStore::Type_Lockpick: mLockpick = src.mLockpick; break;
        case MWWorld::ContainerStore::Type_Miscellaneous: mMiscellaneous = src.mMiscellaneous; break;
        case MWWorld::ContainerStore::Type_Probe: mProbe = src.mProbe; break;
        case MWWorld::ContainerStore::Type_Repair: mRepair = src.mRepair; break;
        case MWWorld::ContainerStore::Type_Weapon: mWeapon = src.mWeapon; break;
        case -1: break;
        default: assert(0);
    }
}

template<class PtrType>
void MWWorld::ContainerStoreIteratorBase<PtrType>::incType()
{
    if (mType==0)
        mType = 1;
    else if (mType!=-1)
    {
        mType <<= 1;

        if (mType>ContainerStore::Type_Last)
            mType = -1;
    }
}

template<class PtrType>
void MWWorld::ContainerStoreIteratorBase<PtrType>::nextType()
{
    while (mType!=-1)
    {
        incType();

        if ((mType & mMask) && mType>0)
            if (resetIterator())
                break;
    }
}

template<class PtrType>
bool MWWorld::ContainerStoreIteratorBase<PtrType>::resetIterator()
{
    switch (mType)
    {
        case ContainerStore::Type_Potion:

            mPotion = mContainer->potions.mList.begin();
            return mPotion!=mContainer->potions.mList.end();

        case ContainerStore::Type_Apparatus:

            mApparatus = mContainer->appas.mList.begin();
            return mApparatus!=mContainer->appas.mList.end();

        case ContainerStore::Type_Armor:

            mArmor = mContainer->armors.mList.begin();
            return mArmor!=mContainer->armors.mList.end();

        case ContainerStore::Type_Book:

            mBook = mContainer->books.mList.begin();
            return mBook!=mContainer->books.mList.end();

        case ContainerStore::Type_Clothing:

            mClothing = mContainer->clothes.mList.begin();
            return mClothing!=mContainer->clothes.mList.end();

        case ContainerStore::Type_Ingredient:

            mIngredient = mContainer->ingreds.mList.begin();
            return mIngredient!=mContainer->ingreds.mList.end();

        case ContainerStore::Type_Light:

            mLight = mContainer->lights.mList.begin();
            return mLight!=mContainer->lights.mList.end();

        case ContainerStore::Type_Lockpick:

            mLockpick = mContainer->lockpicks.mList.begin();
            return mLockpick!=mContainer->lockpicks.mList.end();

        case ContainerStore::Type_Miscellaneous:

            mMiscellaneous = mContainer->miscItems.mList.begin();
            return mMiscellaneous!=mContainer->miscItems.mList.end();

        case ContainerStore::Type_Probe:

            mProbe = mContainer->probes.mList.begin();
            return mProbe!=mContainer->probes.mList.end();

        case ContainerStore::Type_Repair:

            mRepair = mContainer->repairs.mList.begin();
            return mRepair!=mContainer->repairs.mList.end();

        case ContainerStore::Type_Weapon:

            mWeapon = mContainer->weapons.mList.begin();
            return mWeapon!=mContainer->weapons.mList.end();
    }

    return false;
}

template<class PtrType>
bool MWWorld::ContainerStoreIteratorBase<PtrType>::incIterator()
{
    switch (mType)
    {
        case ContainerStore::Type_Potion:

            ++mPotion;
            return mPotion==mContainer->potions.mList.end();

        case ContainerStore::Type_Apparatus:

            ++mApparatus;
            return mApparatus==mContainer->appas.mList.end();

        case ContainerStore::Type_Armor:

            ++mArmor;
            return mArmor==mContainer->armors.mList.end();

        case ContainerStore::Type_Book:

            ++mBook;
            return mBook==mContainer->books.mList.end();

        case ContainerStore::Type_Clothing:

            ++mClothing;
            return mClothing==mContainer->clothes.mList.end();

        case ContainerStore::Type_Ingredient:

            ++mIngredient;
            return mIngredient==mContainer->ingreds.mList.end();

        case ContainerStore::Type_Light:

            ++mLight;
            return mLight==mContainer->lights.mList.end();

        case ContainerStore::Type_Lockpick:

            ++mLockpick;
            return mLockpick==mContainer->lockpicks.mList.end();

        case ContainerStore::Type_Miscellaneous:

            ++mMiscellaneous;
            return mMiscellaneous==mContainer->miscItems.mList.end();

        case ContainerStore::Type_Probe:

            ++mProbe;
            return mProbe==mContainer->probes.mList.end();

        case ContainerStore::Type_Repair:

            ++mRepair;
            return mRepair==mContainer->repairs.mList.end();

        case ContainerStore::Type_Weapon:

            ++mWeapon;
            return mWeapon==mContainer->weapons.mList.end();
    }

    return true;
}


template<class PtrType>
template<class T>
bool MWWorld::ContainerStoreIteratorBase<PtrType>::isEqual (const ContainerStoreIteratorBase<T>& other) const
{
    if (mContainer!=other.mContainer)
        return false;

    if (mType!=other.mType)
        return false;

    switch (mType)
    {
        case ContainerStore::Type_Potion: return mPotion==other.mPotion;
        case ContainerStore::Type_Apparatus: return mApparatus==other.mApparatus;
        case ContainerStore::Type_Armor: return mArmor==other.mArmor;
        case ContainerStore::Type_Book: return mBook==other.mBook;
        case ContainerStore::Type_Clothing: return mClothing==other.mClothing;
        case ContainerStore::Type_Ingredient: return mIngredient==other.mIngredient;
        case ContainerStore::Type_Light: return mLight==other.mLight;
        case ContainerStore::Type_Lockpick: return mLockpick==other.mLockpick;
        case ContainerStore::Type_Miscellaneous: return mMiscellaneous==other.mMiscellaneous;
        case ContainerStore::Type_Probe: return mProbe==other.mProbe;
        case ContainerStore::Type_Repair: return mRepair==other.mRepair;
        case ContainerStore::Type_Weapon: return mWeapon==other.mWeapon;
        case -1: return true;
    }

    return false;  
}

template<class PtrType>
PtrType *MWWorld::ContainerStoreIteratorBase<PtrType>::operator->() const
{
    mPtr = **this;
    return &mPtr;
}

template<class PtrType>
PtrType MWWorld::ContainerStoreIteratorBase<PtrType>::operator*() const
{
    PtrType ptr;

    switch (mType)
    {
        case ContainerStore::Type_Potion: ptr = PtrType (&*mPotion, nullptr); break;
        case ContainerStore::Type_Apparatus: ptr = PtrType (&*mApparatus, nullptr); break;
        case ContainerStore::Type_Armor: ptr = PtrType (&*mArmor, nullptr); break;
        case ContainerStore::Type_Book: ptr = PtrType (&*mBook, nullptr); break;
        case ContainerStore::Type_Clothing: ptr = PtrType (&*mClothing, nullptr); break;
        case ContainerStore::Type_Ingredient: ptr = PtrType (&*mIngredient, nullptr); break;
        case ContainerStore::Type_Light: ptr = PtrType (&*mLight, nullptr); break;
        case ContainerStore::Type_Lockpick: ptr = PtrType (&*mLockpick, nullptr); break;
        case ContainerStore::Type_Miscellaneous: ptr = PtrType (&*mMiscellaneous, nullptr); break;
        case ContainerStore::Type_Probe: ptr = PtrType (&*mProbe, nullptr); break;
        case ContainerStore::Type_Repair: ptr = PtrType (&*mRepair, nullptr); break;
        case ContainerStore::Type_Weapon: ptr = PtrType (&*mWeapon, nullptr); break;
    }

    if (ptr.isEmpty())
        throw std::runtime_error ("invalid iterator");

    ptr.setContainerStore (mContainer);

    return ptr;
}

template<class PtrType>
MWWorld::ContainerStoreIteratorBase<PtrType>& MWWorld::ContainerStoreIteratorBase<PtrType>::operator++()
{
    do
    {
        if (incIterator())
            nextType();
    }
    while (mType!=-1 && !(**this).getRefData().getCount());

    return *this;
}

template<class PtrType>
MWWorld::ContainerStoreIteratorBase<PtrType> MWWorld::ContainerStoreIteratorBase<PtrType>::operator++ (int)
{
    ContainerStoreIteratorBase<PtrType> iter (*this);
    ++*this;
    return iter;
}

template<class PtrType>
MWWorld::ContainerStoreIteratorBase<PtrType>& MWWorld::ContainerStoreIteratorBase<PtrType>::operator= (const ContainerStoreIteratorBase<PtrType>& rhs)
{
    if (this!=&rhs)
    {
        copy(rhs);
    }
    return *this;
}

template<class PtrType>
int MWWorld::ContainerStoreIteratorBase<PtrType>::getType() const
{
    return mType;
}

template<class PtrType>
const MWWorld::ContainerStore *MWWorld::ContainerStoreIteratorBase<PtrType>::getContainerStore() const
{
    return mContainer;
}

template<class PtrType>
MWWorld::ContainerStoreIteratorBase<PtrType>::ContainerStoreIteratorBase (ContainerStoreType container)
: mType (-1), mMask (0), mContainer (container)
{}

template<class PtrType>
MWWorld::ContainerStoreIteratorBase<PtrType>::ContainerStoreIteratorBase (int mask, ContainerStoreType container)
: mType (0), mMask (mask), mContainer (container)
{
    nextType();

    if (mType==-1 || (**this).getRefData().getCount())
        return;

    ++*this;
}

template<class PtrType>
MWWorld::ContainerStoreIteratorBase<PtrType>::ContainerStoreIteratorBase (ContainerStoreType container, typename Iterator<ESM::Potion>::type iterator)
    : mType(MWWorld::ContainerStore::Type_Potion), mMask(MWWorld::ContainerStore::Type_All), mContainer(container), mPotion(iterator){}

template<class PtrType>
MWWorld::ContainerStoreIteratorBase<PtrType>::ContainerStoreIteratorBase (ContainerStoreType container, typename Iterator<ESM::Apparatus>::type iterator)
    : mType(MWWorld::ContainerStore::Type_Apparatus), mMask(MWWorld::ContainerStore::Type_All), mContainer(container), mApparatus(iterator){}

template<class PtrType>
MWWorld::ContainerStoreIteratorBase<PtrType>::ContainerStoreIteratorBase (ContainerStoreType container, typename Iterator<ESM::Armor>::type iterator)
    : mType(MWWorld::ContainerStore::Type_Armor), mMask(MWWorld::ContainerStore::Type_All), mContainer(container), mArmor(iterator){}

template<class PtrType>
MWWorld::ContainerStoreIteratorBase<PtrType>::ContainerStoreIteratorBase (ContainerStoreType container, typename Iterator<ESM::Book>::type iterator)
    : mType(MWWorld::ContainerStore::Type_Book), mMask(MWWorld::ContainerStore::Type_All), mContainer(container), mBook(iterator){}

template<class PtrType>
MWWorld::ContainerStoreIteratorBase<PtrType>::ContainerStoreIteratorBase (ContainerStoreType container, typename Iterator<ESM::Clothing>::type iterator)
    : mType(MWWorld::ContainerStore::Type_Clothing), mMask(MWWorld::ContainerStore::Type_All), mContainer(container), mClothing(iterator){}

template<class PtrType>
MWWorld::ContainerStoreIteratorBase<PtrType>::ContainerStoreIteratorBase (ContainerStoreType container, typename Iterator<ESM::Ingredient>::type iterator)
    : mType(MWWorld::ContainerStore::Type_Ingredient), mMask(MWWorld::ContainerStore::Type_All), mContainer(container), mIngredient(iterator){}

template<class PtrType>
MWWorld::ContainerStoreIteratorBase<PtrType>::ContainerStoreIteratorBase (ContainerStoreType container, typename Iterator<ESM::Light>::type iterator)
    : mType(MWWorld::ContainerStore::Type_Light), mMask(MWWorld::ContainerStore::Type_All), mContainer(container), mLight(iterator){}

template<class PtrType>
MWWorld::ContainerStoreIteratorBase<PtrType>::ContainerStoreIteratorBase (ContainerStoreType container, typename Iterator<ESM::Lockpick>::type iterator)
    : mType(MWWorld::ContainerStore::Type_Lockpick), mMask(MWWorld::ContainerStore::Type_All), mContainer(container), mLockpick(iterator){}

template<class PtrType>
MWWorld::ContainerStoreIteratorBase<PtrType>::ContainerStoreIteratorBase (ContainerStoreType container, typename Iterator<ESM::Miscellaneous>::type iterator)
    : mType(MWWorld::ContainerStore::Type_Miscellaneous), mMask(MWWorld::ContainerStore::Type_All), mContainer(container), mMiscellaneous(iterator){}

template<class PtrType>
MWWorld::ContainerStoreIteratorBase<PtrType>::ContainerStoreIteratorBase (ContainerStoreType container, typename Iterator<ESM::Probe>::type iterator)
    : mType(MWWorld::ContainerStore::Type_Probe), mMask(MWWorld::ContainerStore::Type_All), mContainer(container), mProbe(iterator){}

template<class PtrType>
MWWorld::ContainerStoreIteratorBase<PtrType>::ContainerStoreIteratorBase (ContainerStoreType container, typename Iterator<ESM::Repair>::type iterator)
    : mType(MWWorld::ContainerStore::Type_Repair), mMask(MWWorld::ContainerStore::Type_All), mContainer(container), mRepair(iterator){}

template<class PtrType>
MWWorld::ContainerStoreIteratorBase<PtrType>::ContainerStoreIteratorBase (ContainerStoreType container, typename Iterator<ESM::Weapon>::type iterator)
    : mType(MWWorld::ContainerStore::Type_Weapon), mMask(MWWorld::ContainerStore::Type_All), mContainer(container), mWeapon(iterator){}


template<class T, class U>
bool MWWorld::operator== (const ContainerStoreIteratorBase<T>& left, const ContainerStoreIteratorBase<U>& right)
{
    return left.isEqual (right);
}

template<class T, class U>
bool MWWorld::operator!= (const ContainerStoreIteratorBase<T>& left, const ContainerStoreIteratorBase<U>& right)
{
    return !(left==right);
}

template class MWWorld::ContainerStoreIteratorBase<MWWorld::Ptr>;
template class MWWorld::ContainerStoreIteratorBase<MWWorld::ConstPtr>;

template bool MWWorld::operator== (const ContainerStoreIteratorBase<Ptr>& left, const ContainerStoreIteratorBase<Ptr>& right);
template bool MWWorld::operator!= (const ContainerStoreIteratorBase<Ptr>& left, const ContainerStoreIteratorBase<Ptr>& right);
template bool MWWorld::operator== (const ContainerStoreIteratorBase<ConstPtr>& left, const ContainerStoreIteratorBase<ConstPtr>& right);
template bool MWWorld::operator!= (const ContainerStoreIteratorBase<ConstPtr>& left, const ContainerStoreIteratorBase<ConstPtr>& right);
template bool MWWorld::operator== (const ContainerStoreIteratorBase<ConstPtr>& left, const ContainerStoreIteratorBase<Ptr>& right);
template bool MWWorld::operator!= (const ContainerStoreIteratorBase<ConstPtr>& left, const ContainerStoreIteratorBase<Ptr>& right);
template bool MWWorld::operator== (const ContainerStoreIteratorBase<Ptr>& left, const ContainerStoreIteratorBase<ConstPtr>& right);
template bool MWWorld::operator!= (const ContainerStoreIteratorBase<Ptr>& left, const ContainerStoreIteratorBase<ConstPtr>& right);

template void MWWorld::ContainerStoreIteratorBase<MWWorld::Ptr>::copy(const ContainerStoreIteratorBase<Ptr>& src);
template void MWWorld::ContainerStoreIteratorBase<MWWorld::ConstPtr>::copy(const ContainerStoreIteratorBase<Ptr>& src);
template void MWWorld::ContainerStoreIteratorBase<MWWorld::ConstPtr>::copy(const ContainerStoreIteratorBase<ConstPtr>& src);
