
#include "containerstore.hpp"

#include <cassert>
#include <typeinfo>
#include <stdexcept>

#include <boost/algorithm/string.hpp>

#include <components/esm/loadcont.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"

#include "manualref.hpp"
#include "refdata.hpp"
#include "class.hpp"

namespace
{
    template<typename T>
    float getTotalWeight (const MWWorld::CellRefList<T>& cellRefList)
    {
        float sum = 0;

        for (typename MWWorld::CellRefList<T>::List::const_iterator iter (
            cellRefList.list.begin());
            iter!=cellRefList.list.end();
            ++iter)
        {
            if (iter->mData.getCount()>0)
                sum += iter->mData.getCount()*iter->base->mData.mWeight;
        }

        return sum;
    }

    bool compare_string_ci(std::string str1, std::string str2)
    {
        boost::algorithm::to_lower(str1);
        return str1 == str2;
    }
}

MWWorld::ContainerStore::ContainerStore() : mStateId (0), mCachedWeight (0), mWeightUpToDate (false) {}

MWWorld::ContainerStore::~ContainerStore() {}

MWWorld::ContainerStoreIterator MWWorld::ContainerStore::begin (int mask)
{
    return ContainerStoreIterator (mask, this);
}

MWWorld::ContainerStoreIterator MWWorld::ContainerStore::end()
{
    return ContainerStoreIterator (this);
}

bool MWWorld::ContainerStore::stacks(const Ptr& ptr1, const Ptr& ptr2)
{
    /// \todo add current weapon/armor health, remaining lockpick/repair uses, current enchantment charge here as soon as they are implemented
    if (  ptr1.mCellRef->mRefID == ptr2.mCellRef->mRefID
        && MWWorld::Class::get(ptr1).getScript(ptr1) == "" // item with a script never stacks
        && MWWorld::Class::get(ptr1).getEnchantment(ptr1) == "" // item with enchantment never stacks (we could revisit this later, but for now it makes selecting items in the spell window much easier)
        && ptr1.mCellRef->mOwner == ptr2.mCellRef->mOwner
        && ptr1.mCellRef->mSoul == ptr2.mCellRef->mSoul
        && ptr1.mCellRef->mCharge == ptr2.mCellRef->mCharge)
        return true;

    return false;
}

MWWorld::ContainerStoreIterator MWWorld::ContainerStore::add (const Ptr& ptr)
{
    int type = getType(ptr);

    // gold needs special handling: when it is inserted into a container, the base object automatically becomes Gold_001
    // this ensures that gold piles of different sizes stack with each other (also, several scripts rely on Gold_001 for detecting player gold)
    if (MWWorld::Class::get(ptr).getName(ptr) == MWBase::Environment::get().getWorld()->getStore().gameSettings.find("sGold")->getString())
    {
        MWWorld::LiveCellRef<ESM::Miscellaneous> *gold =
            ptr.get<ESM::Miscellaneous>();

        if (compare_string_ci(gold->ref.mRefID, "gold_001")
            || compare_string_ci(gold->ref.mRefID, "gold_005")
            || compare_string_ci(gold->ref.mRefID, "gold_010")
            || compare_string_ci(gold->ref.mRefID, "gold_025")
            || compare_string_ci(gold->ref.mRefID, "gold_100"))
        {
            MWWorld::ManualRef ref(MWBase::Environment::get().getWorld()->getStore(), "Gold_001");

            int count = (ptr.getRefData().getCount() == 1) ? gold->base->mData.mValue : ptr.getRefData().getCount();
            ref.getPtr().getRefData().setCount(count);
            for (MWWorld::ContainerStoreIterator iter (begin(type)); iter!=end(); ++iter)
            {
                if (compare_string_ci((*iter).get<ESM::Miscellaneous>()->ref.mRefID, "gold_001"))
                {
                    (*iter).getRefData().setCount( (*iter).getRefData().getCount() + count);
                    flagAsModified();
                    return iter;
                }
            }

            return addImpl(ref.getPtr());
        }
    }

    // determine whether to stack or not
    for (MWWorld::ContainerStoreIterator iter (begin(type)); iter!=end(); ++iter)
    {
        if (stacks(*iter, ptr))
        {
            // stack
            iter->getRefData().setCount( iter->getRefData().getCount() + ptr.getRefData().getCount() );

            flagAsModified();
            return iter;
        }
    }
    // if we got here, this means no stacking
    return addImpl(ptr);
}

MWWorld::ContainerStoreIterator MWWorld::ContainerStore::addImpl (const Ptr& ptr)
{
    ContainerStoreIterator it = begin();

    switch (getType(ptr))
    {
        case Type_Potion: potions.list.push_back (*ptr.get<ESM::Potion>()); it = ContainerStoreIterator(this, --potions.list.end()); break;
        case Type_Apparatus: appas.list.push_back (*ptr.get<ESM::Apparatus>()); it = ContainerStoreIterator(this, --appas.list.end()); break;
        case Type_Armor: armors.list.push_back (*ptr.get<ESM::Armor>()); it = ContainerStoreIterator(this, --armors.list.end()); break;
        case Type_Book: books.list.push_back (*ptr.get<ESM::Book>()); it = ContainerStoreIterator(this, --books.list.end()); break;
        case Type_Clothing: clothes.list.push_back (*ptr.get<ESM::Clothing>()); it = ContainerStoreIterator(this, --clothes.list.end()); break;
        case Type_Ingredient: ingreds.list.push_back (*ptr.get<ESM::Ingredient>()); it = ContainerStoreIterator(this, --ingreds.list.end()); break;
        case Type_Light: lights.list.push_back (*ptr.get<ESM::Light>()); it = ContainerStoreIterator(this, --lights.list.end()); break;
        case Type_Lockpick: lockpicks.list.push_back (*ptr.get<ESM::Tool>()); it = ContainerStoreIterator(this, --lockpicks.list.end()); break;
        case Type_Miscellaneous: miscItems.list.push_back (*ptr.get<ESM::Miscellaneous>()); it = ContainerStoreIterator(this, --miscItems.list.end()); break;
        case Type_Probe: probes.list.push_back (*ptr.get<ESM::Probe>()); it = ContainerStoreIterator(this, --probes.list.end()); break;
        case Type_Repair: repairs.list.push_back (*ptr.get<ESM::Repair>()); it = ContainerStoreIterator(this, --repairs.list.end()); break;
        case Type_Weapon: weapons.list.push_back (*ptr.get<ESM::Weapon>()); it = ContainerStoreIterator(this, --weapons.list.end()); break;
    }

    flagAsModified();
    return it;
}

void MWWorld::ContainerStore::fill (const ESM::InventoryList& items, const ESMS::ESMStore& store)
{
    for (std::vector<ESM::ContItem>::const_iterator iter (items.mList.begin()); iter!=items.mList.end();
        ++iter)
    {
        ManualRef ref (store, iter->mItem.toString());

        if (ref.getPtr().getTypeName()==typeid (ESM::ItemLevList).name())
        {
            /// \todo implement leveled item lists
            continue;
        }

        ref.getPtr().getRefData().setCount (std::abs(iter->mCount)); /// \todo implement item restocking (indicated by negative count)
        add (ref.getPtr());
    }

    flagAsModified();
}

void MWWorld::ContainerStore::clear()
{
    potions.list.clear();
    appas.list.clear();
    armors.list.clear();
    books.list.clear();
    clothes.list.clear();
    ingreds.list.clear();
    lights.list.clear();
    lockpicks.list.clear();
    miscItems.list.clear();
    probes.list.clear();
    repairs.list.clear();
    weapons.list.clear();

    flagAsModified();
}

void MWWorld::ContainerStore::flagAsModified()
{
    ++mStateId;
    mWeightUpToDate = false;
}

int MWWorld::ContainerStore::getStateId() const
{
    return mStateId;
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

int MWWorld::ContainerStore::getType (const Ptr& ptr)
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

    if (ptr.getTypeName()==typeid (ESM::Tool).name())
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
        "Object of type " + ptr.getTypeName() + " can not be placed into a container");
}


MWWorld::ContainerStoreIterator::ContainerStoreIterator (ContainerStore *container)
: mType (-1), mMask (0), mContainer (container)
{}

MWWorld::ContainerStoreIterator::ContainerStoreIterator (int mask, ContainerStore *container)
: mType (0), mMask (mask), mContainer (container)
{
    nextType();

    if (mType==-1 || (**this).getRefData().getCount())
        return;

    ++*this;
}

MWWorld::ContainerStoreIterator::ContainerStoreIterator (ContainerStore *container, MWWorld::CellRefList<ESM::Potion>::List::iterator iterator)
    : mType(MWWorld::ContainerStore::Type_Potion), mMask(MWWorld::ContainerStore::Type_All), mContainer(container), mPotion(iterator){}
MWWorld::ContainerStoreIterator::ContainerStoreIterator (ContainerStore *container, MWWorld::CellRefList<ESM::Apparatus>::List::iterator iterator)
    : mType(MWWorld::ContainerStore::Type_Apparatus), mMask(MWWorld::ContainerStore::Type_All), mContainer(container), mApparatus(iterator){}
MWWorld::ContainerStoreIterator::ContainerStoreIterator (ContainerStore *container, MWWorld::CellRefList<ESM::Armor>::List::iterator iterator)
    : mType(MWWorld::ContainerStore::Type_Armor), mMask(MWWorld::ContainerStore::Type_All), mContainer(container), mArmor(iterator){}
MWWorld::ContainerStoreIterator::ContainerStoreIterator (ContainerStore *container, MWWorld::CellRefList<ESM::Book>::List::iterator iterator)
    : mType(MWWorld::ContainerStore::Type_Book), mMask(MWWorld::ContainerStore::Type_All), mContainer(container), mBook(iterator){}
MWWorld::ContainerStoreIterator::ContainerStoreIterator (ContainerStore *container, MWWorld::CellRefList<ESM::Clothing>::List::iterator iterator)
    : mType(MWWorld::ContainerStore::Type_Clothing), mMask(MWWorld::ContainerStore::Type_All), mContainer(container), mClothing(iterator){}
MWWorld::ContainerStoreIterator::ContainerStoreIterator (ContainerStore *container, MWWorld::CellRefList<ESM::Ingredient>::List::iterator iterator)
    : mType(MWWorld::ContainerStore::Type_Ingredient), mMask(MWWorld::ContainerStore::Type_All), mContainer(container), mIngredient(iterator){}
MWWorld::ContainerStoreIterator::ContainerStoreIterator (ContainerStore *container, MWWorld::CellRefList<ESM::Light>::List::iterator iterator)
    : mType(MWWorld::ContainerStore::Type_Light), mMask(MWWorld::ContainerStore::Type_All), mContainer(container), mLight(iterator){}
MWWorld::ContainerStoreIterator::ContainerStoreIterator (ContainerStore *container, MWWorld::CellRefList<ESM::Tool>::List::iterator iterator)
    : mType(MWWorld::ContainerStore::Type_Lockpick), mMask(MWWorld::ContainerStore::Type_All), mContainer(container), mLockpick(iterator){}
MWWorld::ContainerStoreIterator::ContainerStoreIterator (ContainerStore *container, MWWorld::CellRefList<ESM::Miscellaneous>::List::iterator iterator)
    : mType(MWWorld::ContainerStore::Type_Miscellaneous), mMask(MWWorld::ContainerStore::Type_All), mContainer(container), mMiscellaneous(iterator){}
MWWorld::ContainerStoreIterator::ContainerStoreIterator (ContainerStore *container, MWWorld::CellRefList<ESM::Probe>::List::iterator iterator)
    : mType(MWWorld::ContainerStore::Type_Probe), mMask(MWWorld::ContainerStore::Type_All), mContainer(container), mProbe(iterator){}
MWWorld::ContainerStoreIterator::ContainerStoreIterator (ContainerStore *container, MWWorld::CellRefList<ESM::Repair>::List::iterator iterator)
    : mType(MWWorld::ContainerStore::Type_Repair), mMask(MWWorld::ContainerStore::Type_All), mContainer(container), mRepair(iterator){}
MWWorld::ContainerStoreIterator::ContainerStoreIterator (ContainerStore *container, MWWorld::CellRefList<ESM::Weapon>::List::iterator iterator)
    : mType(MWWorld::ContainerStore::Type_Weapon), mMask(MWWorld::ContainerStore::Type_All), mContainer(container), mWeapon(iterator){}

void MWWorld::ContainerStoreIterator::incType()
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

void MWWorld::ContainerStoreIterator::nextType()
{
    while (mType!=-1)
    {
        incType();

        if ((mType & mMask) && mType>0)
            if (resetIterator())
                break;
    }
}

bool MWWorld::ContainerStoreIterator::resetIterator()
{
    switch (mType)
    {
        case ContainerStore::Type_Potion:

            mPotion = mContainer->potions.list.begin();
            return mPotion!=mContainer->potions.list.end();

        case ContainerStore::Type_Apparatus:

            mApparatus = mContainer->appas.list.begin();
            return mApparatus!=mContainer->appas.list.end();

        case ContainerStore::Type_Armor:

            mArmor = mContainer->armors.list.begin();
            return mArmor!=mContainer->armors.list.end();

        case ContainerStore::Type_Book:

            mBook = mContainer->books.list.begin();
            return mBook!=mContainer->books.list.end();

        case ContainerStore::Type_Clothing:

            mClothing = mContainer->clothes.list.begin();
            return mClothing!=mContainer->clothes.list.end();

        case ContainerStore::Type_Ingredient:

            mIngredient = mContainer->ingreds.list.begin();
            return mIngredient!=mContainer->ingreds.list.end();

        case ContainerStore::Type_Light:

            mLight = mContainer->lights.list.begin();
            return mLight!=mContainer->lights.list.end();

        case ContainerStore::Type_Lockpick:

            mLockpick = mContainer->lockpicks.list.begin();
            return mLockpick!=mContainer->lockpicks.list.end();

        case ContainerStore::Type_Miscellaneous:

            mMiscellaneous = mContainer->miscItems.list.begin();
            return mMiscellaneous!=mContainer->miscItems.list.end();

        case ContainerStore::Type_Probe:

            mProbe = mContainer->probes.list.begin();
            return mProbe!=mContainer->probes.list.end();

        case ContainerStore::Type_Repair:

            mRepair = mContainer->repairs.list.begin();
            return mRepair!=mContainer->repairs.list.end();

        case ContainerStore::Type_Weapon:

            mWeapon = mContainer->weapons.list.begin();
            return mWeapon!=mContainer->weapons.list.end();
    }

    return false;
}

bool MWWorld::ContainerStoreIterator::incIterator()
{
    switch (mType)
    {
        case ContainerStore::Type_Potion:

            ++mPotion;
            return mPotion==mContainer->potions.list.end();

        case ContainerStore::Type_Apparatus:

            ++mApparatus;
            return mApparatus==mContainer->appas.list.end();

        case ContainerStore::Type_Armor:

            ++mArmor;
            return mArmor==mContainer->armors.list.end();

        case ContainerStore::Type_Book:

            ++mBook;
            return mBook==mContainer->books.list.end();

        case ContainerStore::Type_Clothing:

            ++mClothing;
            return mClothing==mContainer->clothes.list.end();

        case ContainerStore::Type_Ingredient:

            ++mIngredient;
            return mIngredient==mContainer->ingreds.list.end();

        case ContainerStore::Type_Light:

            ++mLight;
            return mLight==mContainer->lights.list.end();

        case ContainerStore::Type_Lockpick:

            ++mLockpick;
            return mLockpick==mContainer->lockpicks.list.end();

        case ContainerStore::Type_Miscellaneous:

            ++mMiscellaneous;
            return mMiscellaneous==mContainer->miscItems.list.end();

        case ContainerStore::Type_Probe:

            ++mProbe;
            return mProbe==mContainer->probes.list.end();

        case ContainerStore::Type_Repair:

            ++mRepair;
            return mRepair==mContainer->repairs.list.end();

        case ContainerStore::Type_Weapon:

            ++mWeapon;
            return mWeapon==mContainer->weapons.list.end();
    }

    return true;
}

MWWorld::Ptr *MWWorld::ContainerStoreIterator::operator->() const
{
    mPtr = **this;
    return &mPtr;
}

MWWorld::Ptr MWWorld::ContainerStoreIterator::operator*() const
{
    Ptr ptr;

    switch (mType)
    {
        case ContainerStore::Type_Potion: ptr = MWWorld::Ptr (&*mPotion, 0); break;
        case ContainerStore::Type_Apparatus: ptr = MWWorld::Ptr (&*mApparatus, 0); break;
        case ContainerStore::Type_Armor: ptr = MWWorld::Ptr (&*mArmor, 0); break;
        case ContainerStore::Type_Book: ptr = MWWorld::Ptr (&*mBook, 0); break;
        case ContainerStore::Type_Clothing: ptr = MWWorld::Ptr (&*mClothing, 0); break;
        case ContainerStore::Type_Ingredient: ptr = MWWorld::Ptr (&*mIngredient, 0); break;
        case ContainerStore::Type_Light: ptr = MWWorld::Ptr (&*mLight, 0); break;
        case ContainerStore::Type_Lockpick: ptr = MWWorld::Ptr (&*mLockpick, 0); break;
        case ContainerStore::Type_Miscellaneous: ptr = MWWorld::Ptr (&*mMiscellaneous, 0); break;
        case ContainerStore::Type_Probe: ptr = MWWorld::Ptr (&*mProbe, 0); break;
        case ContainerStore::Type_Repair: ptr = MWWorld::Ptr (&*mRepair, 0); break;
        case ContainerStore::Type_Weapon: ptr = MWWorld::Ptr (&*mWeapon, 0); break;
    }

    if (ptr.isEmpty())
        throw std::runtime_error ("invalid iterator");

    ptr.setContainerStore (mContainer);

    return ptr;
}

MWWorld::ContainerStoreIterator& MWWorld::ContainerStoreIterator::operator++()
{
    do
    {
        if (incIterator())
            nextType();
    }
    while (mType!=-1 && !(**this).getRefData().getCount());

    return *this;
}

MWWorld::ContainerStoreIterator MWWorld::ContainerStoreIterator::operator++ (int)
{
    ContainerStoreIterator iter (*this);
    ++*this;
    return iter;
}

bool MWWorld::ContainerStoreIterator::isEqual (const ContainerStoreIterator& iter) const
{
    assert (mContainer==iter.mContainer);

    if (mType!=iter.mType)
        return false;

    switch (mType)
    {
        case ContainerStore::Type_Potion: return mPotion==iter.mPotion;
        case ContainerStore::Type_Apparatus: return mApparatus==iter.mApparatus;
        case ContainerStore::Type_Armor: return mArmor==iter.mArmor;
        case ContainerStore::Type_Book: return mBook==iter.mBook;
        case ContainerStore::Type_Clothing: return mClothing==iter.mClothing;
        case ContainerStore::Type_Ingredient: return mIngredient==iter.mIngredient;
        case ContainerStore::Type_Light: return mLight==iter.mLight;
        case ContainerStore::Type_Lockpick: return mLockpick==iter.mLockpick;
        case ContainerStore::Type_Miscellaneous: return mMiscellaneous==iter.mMiscellaneous;
        case ContainerStore::Type_Probe: return mProbe==iter.mProbe;
        case ContainerStore::Type_Repair: return mRepair==iter.mRepair;
        case ContainerStore::Type_Weapon: return mWeapon==iter.mWeapon;
        case -1: return true;
    }

    return false;
}

int MWWorld::ContainerStoreIterator::getType() const
{
    return mType;
}

const MWWorld::ContainerStore *MWWorld::ContainerStoreIterator::getContainerStore() const
{
    return mContainer;
}

bool MWWorld::operator== (const ContainerStoreIterator& left, const ContainerStoreIterator& right)
{
    return left.isEqual (right);
}

bool MWWorld::operator!= (const ContainerStoreIterator& left, const ContainerStoreIterator& right)
{
    return !(left==right);
}
