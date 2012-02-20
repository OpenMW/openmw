
#include "containerstore.hpp"

#include <cassert>

MWWorld::ContainerStoreIterator MWWorld::ContainerStore::begin (int mask)
{
    return ContainerStoreIterator (mask, this);
}

MWWorld::ContainerStoreIterator MWWorld::ContainerStore::end()
{
    return ContainerStoreIterator();
}



MWWorld::ContainerStoreIterator::ContainerStoreIterator() : mType (-1), mMask (0), mContainer (0)
{}

MWWorld::ContainerStoreIterator::ContainerStoreIterator (int mask, ContainerStore *container)
: mType (0), mMask (mask), mContainer (container)
{
    nextType();
}

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

        if (mType & mMask)
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
    switch (mType)
    {
        case ContainerStore::Type_Potion: return MWWorld::Ptr (&*mPotion, 0);
        case ContainerStore::Type_Apparatus: return MWWorld::Ptr (&*mApparatus, 0);
        case ContainerStore::Type_Armor: return MWWorld::Ptr (&*mArmor, 0);
        case ContainerStore::Type_Book: return MWWorld::Ptr (&*mBook, 0);
        case ContainerStore::Type_Clothing: return MWWorld::Ptr (&*mClothing, 0);
        case ContainerStore::Type_Ingredient: return MWWorld::Ptr (&*mIngredient, 0);
        case ContainerStore::Type_Light: return MWWorld::Ptr (&*mLight, 0);
        case ContainerStore::Type_Lockpick: return MWWorld::Ptr (&*mLockpick, 0);
        case ContainerStore::Type_Miscellaneous: return MWWorld::Ptr (&*mMiscellaneous, 0);
        case ContainerStore::Type_Probe: return MWWorld::Ptr (&*mProbe, 0);
        case ContainerStore::Type_Repair: return MWWorld::Ptr (&*mRepair, 0);
        case ContainerStore::Type_Weapon: return MWWorld::Ptr (&*mWeapon, 0);
    }

    return MWWorld::Ptr();
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
    }

    return false;
}

int MWWorld::ContainerStoreIterator::getType() const
{
    return mType;
}

bool MWWorld::operator== (const ContainerStoreIterator& left, const ContainerStoreIterator& right)
{
    return left.isEqual (right);
}

bool MWWorld::operator!= (const ContainerStoreIterator& left, const ContainerStoreIterator& right)
{
    return !(left==right);
}
