
#include "refiddata.hpp"

#include <cassert>

#include <components/misc/stringops.hpp>

CSMWorld::RefIdDataContainerBase::~RefIdDataContainerBase() {}

CSMWorld::RefIdData::RefIdData()
{
    mRecordContainers.insert (std::make_pair (UniversalId::Type_Activator, &mActivators));
    mRecordContainers.insert (std::make_pair (UniversalId::Type_Potion, &mPotions));
    mRecordContainers.insert (std::make_pair (UniversalId::Type_Apparatus, &mApparati));
    mRecordContainers.insert (std::make_pair (UniversalId::Type_Armor, &mArmors));
    mRecordContainers.insert (std::make_pair (UniversalId::Type_Book, &mBooks));
    mRecordContainers.insert (std::make_pair (UniversalId::Type_Clothing, &mClothing));
    mRecordContainers.insert (std::make_pair (UniversalId::Type_Container, &mContainers));
    mRecordContainers.insert (std::make_pair (UniversalId::Type_Creature, &mCreatures));
    mRecordContainers.insert (std::make_pair (UniversalId::Type_Door, &mDoors));
    mRecordContainers.insert (std::make_pair (UniversalId::Type_Ingredient, &mIngredients));
    mRecordContainers.insert (std::make_pair (UniversalId::Type_CreatureLevelledList,
        &mCreatureLevelledLists));
    mRecordContainers.insert (std::make_pair (UniversalId::Type_ItemLevelledList, &mItemLevelledLists));
    mRecordContainers.insert (std::make_pair (UniversalId::Type_Light, &mLights));
    mRecordContainers.insert (std::make_pair (UniversalId::Type_Lockpick, &mLockpicks));
    mRecordContainers.insert (std::make_pair (UniversalId::Type_Miscellaneous, &mMiscellaneous));
    mRecordContainers.insert (std::make_pair (UniversalId::Type_Npc, &mNpcs));
    mRecordContainers.insert (std::make_pair (UniversalId::Type_Probe, &mProbes));
    mRecordContainers.insert (std::make_pair (UniversalId::Type_Repair, &mRepairs));
    mRecordContainers.insert (std::make_pair (UniversalId::Type_Static, &mStatics));
    mRecordContainers.insert (std::make_pair (UniversalId::Type_Weapon, &mWeapons));
}

CSMWorld::RefIdData::LocalIndex CSMWorld::RefIdData::globalToLocalIndex (int index) const
{
    for (std::map<UniversalId::Type, RefIdDataContainerBase *>::const_iterator iter (
        mRecordContainers.begin()); iter!=mRecordContainers.end(); ++iter)
    {
        if (index<iter->second->getSize())
            return LocalIndex (index, iter->first);

        index -= iter->second->getSize();
    }

    throw std::runtime_error ("RefIdData index out of range");
}

int CSMWorld::RefIdData::localToGlobalIndex (const LocalIndex& index)
    const
{
    std::map<UniversalId::Type, RefIdDataContainerBase *>::const_iterator end =
        mRecordContainers.find (index.second);

    if (end==mRecordContainers.end())
        throw std::logic_error ("invalid local index type");

    int globalIndex = index.first;

    for (std::map<UniversalId::Type, RefIdDataContainerBase *>::const_iterator iter (
        mRecordContainers.begin()); iter!=end; ++iter)
        globalIndex += iter->second->getSize();

    return globalIndex;
}

CSMWorld::RefIdData::LocalIndex CSMWorld::RefIdData::searchId (
    const std::string& id) const
{
    std::string id2 = Misc::StringUtils::lowerCase (id);

    std::map<std::string, std::pair<int, UniversalId::Type> >::const_iterator iter = mIndex.find (id2);

    if (iter==mIndex.end())
        return std::make_pair (-1, CSMWorld::UniversalId::Type_None);

    return iter->second;
}

void CSMWorld::RefIdData::erase (int index, int count)
{
    LocalIndex localIndex = globalToLocalIndex (index);

    std::map<UniversalId::Type, RefIdDataContainerBase *>::const_iterator iter =
        mRecordContainers.find (localIndex.second);

    while (count>0 && iter!=mRecordContainers.end())
    {
        int size = iter->second->getSize();

        if (localIndex.first+count>size)
        {
            erase (localIndex, size-localIndex.first);
            count -= size-localIndex.first;

            ++iter;

            if (iter==mRecordContainers.end())
                throw std::runtime_error ("invalid count value for erase operation");

            localIndex.first = 0;
            localIndex.second = iter->first;
        }
        else
        {
            erase (localIndex, count);
            count = 0;
        }
    }
}

const CSMWorld::RecordBase& CSMWorld::RefIdData::getRecord (const LocalIndex& index) const
{
    std::map<UniversalId::Type, RefIdDataContainerBase *>::const_iterator iter =
        mRecordContainers.find (index.second);

    if (iter==mRecordContainers.end())
        throw std::logic_error ("invalid local index type");

    return iter->second->getRecord (index.first);
}

CSMWorld::RecordBase& CSMWorld::RefIdData::getRecord (const LocalIndex& index)
{
    std::map<UniversalId::Type, RefIdDataContainerBase *>::iterator iter =
        mRecordContainers.find (index.second);

    if (iter==mRecordContainers.end())
        throw std::logic_error ("invalid local index type");

    return iter->second->getRecord (index.first);
}

void CSMWorld::RefIdData::appendRecord (UniversalId::Type type, const std::string& id)
{
    std::map<UniversalId::Type, RefIdDataContainerBase *>::iterator iter =
        mRecordContainers.find (type);

    if (iter==mRecordContainers.end())
        throw std::logic_error ("invalid local index type");

    iter->second->appendRecord (id);

    mIndex.insert (std::make_pair (Misc::StringUtils::lowerCase (id),
        LocalIndex (iter->second->getSize()-1, type)));
}

int CSMWorld::RefIdData::getAppendIndex (UniversalId::Type type) const
{
    int index = 0;

    for (std::map<UniversalId::Type, RefIdDataContainerBase *>::const_iterator iter (
        mRecordContainers.begin()); iter!=mRecordContainers.end(); ++iter)
    {
        index += iter->second->getSize();

        if (type==iter->first)
            break;
    }

    return index;
}

void CSMWorld::RefIdData::load (const LocalIndex& index, ESM::ESMReader& reader, bool base)
{
    std::map<UniversalId::Type, RefIdDataContainerBase *>::iterator iter =
        mRecordContainers.find (index.second);

    if (iter==mRecordContainers.end())
        throw std::logic_error ("invalid local index type");

    iter->second->load (index.first, reader, base);
}

void CSMWorld::RefIdData::erase (const LocalIndex& index, int count)
{
    std::map<UniversalId::Type, RefIdDataContainerBase *>::iterator iter =
        mRecordContainers.find (index.second);

    if (iter==mRecordContainers.end())
        throw std::logic_error ("invalid local index type");

    for (int i=index.first; i<index.first+count; ++i)
    {
        std::map<std::string, LocalIndex>::iterator result =
            mIndex.find (Misc::StringUtils::lowerCase (iter->second->getId (i)));

        if (result!=mIndex.end())
            mIndex.erase (result);
    }

    iter->second->erase (index.first, count);
}

int CSMWorld::RefIdData::getSize() const
{
    return mIndex.size();
}
