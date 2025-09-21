#include "refiddata.hpp"

#include <apps/opencs/model/world/record.hpp>
#include <apps/opencs/model/world/universalid.hpp>

#include <components/misc/strings/lower.hpp>

#include <memory>
#include <string_view>
#include <type_traits>

namespace ESM
{
    class ESMWriter;
}

ESM::RefId CSMWorld::RefIdData::getRecordId(const CSMWorld::RefIdData::LocalIndex& index) const
{
    std::map<UniversalId::Type, RefIdDataContainerBase*>::const_iterator found = mRecordContainers.find(index.second);

    if (found == mRecordContainers.end())
        throw std::logic_error("invalid local index type");

    return found->second->getId(index.first);
}

CSMWorld::RefIdData::RefIdData()
{
    mRecordContainers.insert(std::make_pair(UniversalId::Type_Activator, &mActivators));
    mRecordContainers.insert(std::make_pair(UniversalId::Type_Potion, &mPotions));
    mRecordContainers.insert(std::make_pair(UniversalId::Type_Apparatus, &mApparati));
    mRecordContainers.insert(std::make_pair(UniversalId::Type_Armor, &mArmors));
    mRecordContainers.insert(std::make_pair(UniversalId::Type_Book, &mBooks));
    mRecordContainers.insert(std::make_pair(UniversalId::Type_Clothing, &mClothing));
    mRecordContainers.insert(std::make_pair(UniversalId::Type_Container, &mContainers));
    mRecordContainers.insert(std::make_pair(UniversalId::Type_Creature, &mCreatures));
    mRecordContainers.insert(std::make_pair(UniversalId::Type_Door, &mDoors));
    mRecordContainers.insert(std::make_pair(UniversalId::Type_Ingredient, &mIngredients));
    mRecordContainers.insert(std::make_pair(UniversalId::Type_CreatureLevelledList, &mCreatureLevelledLists));
    mRecordContainers.insert(std::make_pair(UniversalId::Type_ItemLevelledList, &mItemLevelledLists));
    mRecordContainers.insert(std::make_pair(UniversalId::Type_Light, &mLights));
    mRecordContainers.insert(std::make_pair(UniversalId::Type_Lockpick, &mLockpicks));
    mRecordContainers.insert(std::make_pair(UniversalId::Type_Miscellaneous, &mMiscellaneous));
    mRecordContainers.insert(std::make_pair(UniversalId::Type_Npc, &mNpcs));
    mRecordContainers.insert(std::make_pair(UniversalId::Type_Probe, &mProbes));
    mRecordContainers.insert(std::make_pair(UniversalId::Type_Repair, &mRepairs));
    mRecordContainers.insert(std::make_pair(UniversalId::Type_Static, &mStatics));
    mRecordContainers.insert(std::make_pair(UniversalId::Type_Weapon, &mWeapons));
}

CSMWorld::RefIdData::LocalIndex CSMWorld::RefIdData::globalToLocalIndex(int index) const
{
    for (std::map<UniversalId::Type, RefIdDataContainerBase*>::const_iterator iter(mRecordContainers.begin());
         iter != mRecordContainers.end(); ++iter)
    {
        if (index < iter->second->getSize())
            return LocalIndex(index, iter->first);

        index -= iter->second->getSize();
    }

    throw std::runtime_error("RefIdData index out of range");
}

int CSMWorld::RefIdData::localToGlobalIndex(const LocalIndex& index) const
{
    std::map<UniversalId::Type, RefIdDataContainerBase*>::const_iterator end = mRecordContainers.find(index.second);

    if (end == mRecordContainers.end())
        throw std::logic_error("invalid local index type");

    int globalIndex = index.first;

    for (std::map<UniversalId::Type, RefIdDataContainerBase*>::const_iterator iter(mRecordContainers.begin());
         iter != end; ++iter)
        globalIndex += iter->second->getSize();

    return globalIndex;
}

CSMWorld::RefIdData::LocalIndex CSMWorld::RefIdData::searchId(const ESM::RefId& id) const
{
    auto iter = mIndex.find(id);

    if (iter == mIndex.end())
        return std::make_pair(-1, CSMWorld::UniversalId::Type_None);

    return iter->second;
}

unsigned int CSMWorld::RefIdData::getRecordFlags(const ESM::RefId& id) const
{
    LocalIndex localIndex = searchId(id);

    switch (localIndex.second)
    {
        case UniversalId::Type_Activator:
            return mActivators.getRecordFlags(localIndex.first);
        case UniversalId::Type_Potion:
            return mPotions.getRecordFlags(localIndex.first);
        case UniversalId::Type_Apparatus:
            return mApparati.getRecordFlags(localIndex.first);
        case UniversalId::Type_Armor:
            return mArmors.getRecordFlags(localIndex.first);
        case UniversalId::Type_Book:
            return mBooks.getRecordFlags(localIndex.first);
        case UniversalId::Type_Clothing:
            return mClothing.getRecordFlags(localIndex.first);
        case UniversalId::Type_Container:
            return mContainers.getRecordFlags(localIndex.first);
        case UniversalId::Type_Creature:
            return mCreatures.getRecordFlags(localIndex.first);
        case UniversalId::Type_Door:
            return mDoors.getRecordFlags(localIndex.first);
        case UniversalId::Type_Ingredient:
            return mIngredients.getRecordFlags(localIndex.first);
        case UniversalId::Type_CreatureLevelledList:
            return mCreatureLevelledLists.getRecordFlags(localIndex.first);
        case UniversalId::Type_ItemLevelledList:
            return mItemLevelledLists.getRecordFlags(localIndex.first);
        case UniversalId::Type_Light:
            return mLights.getRecordFlags(localIndex.first);
        case UniversalId::Type_Lockpick:
            return mLockpicks.getRecordFlags(localIndex.first);
        case UniversalId::Type_Miscellaneous:
            return mMiscellaneous.getRecordFlags(localIndex.first);
        case UniversalId::Type_Npc:
            return mNpcs.getRecordFlags(localIndex.first);
        case UniversalId::Type_Probe:
            return mProbes.getRecordFlags(localIndex.first);
        case UniversalId::Type_Repair:
            return mRepairs.getRecordFlags(localIndex.first);
        case UniversalId::Type_Static:
            return mStatics.getRecordFlags(localIndex.first);
        case UniversalId::Type_Weapon:
            return mWeapons.getRecordFlags(localIndex.first);
        default:
            break;
    }

    return 0;
}

void CSMWorld::RefIdData::erase(int index, int count)
{
    LocalIndex localIndex = globalToLocalIndex(index);

    std::map<UniversalId::Type, RefIdDataContainerBase*>::const_iterator iter
        = mRecordContainers.find(localIndex.second);

    while (count > 0 && iter != mRecordContainers.end())
    {
        int size = iter->second->getSize();

        if (localIndex.first + count > size)
        {
            erase(localIndex, size - localIndex.first);
            count -= size - localIndex.first;

            ++iter;

            if (iter == mRecordContainers.end())
                throw std::runtime_error("invalid count value for erase operation");

            localIndex.first = 0;
            localIndex.second = iter->first;
        }
        else
        {
            erase(localIndex, count);
            count = 0;
        }
    }
}

const CSMWorld::RecordBase& CSMWorld::RefIdData::getRecord(const LocalIndex& index) const
{
    std::map<UniversalId::Type, RefIdDataContainerBase*>::const_iterator iter = mRecordContainers.find(index.second);

    if (iter == mRecordContainers.end())
        throw std::logic_error("invalid local index type");

    return iter->second->getRecord(index.first);
}

CSMWorld::RecordBase& CSMWorld::RefIdData::getRecord(const LocalIndex& index)
{
    std::map<UniversalId::Type, RefIdDataContainerBase*>::iterator iter = mRecordContainers.find(index.second);

    if (iter == mRecordContainers.end())
        throw std::logic_error("invalid local index type");

    return iter->second->getRecord(index.first);
}

void CSMWorld::RefIdData::appendRecord(UniversalId::Type type, const ESM::RefId& id, bool base)
{
    std::map<UniversalId::Type, RefIdDataContainerBase*>::iterator iter = mRecordContainers.find(type);

    if (iter == mRecordContainers.end())
        throw std::logic_error("invalid local index type");

    iter->second->appendRecord(id, base);

    mIndex.insert(std::make_pair(id, LocalIndex(iter->second->getSize() - 1, type)));
}

int CSMWorld::RefIdData::getAppendIndex(UniversalId::Type type) const
{
    int index = 0;

    for (std::map<UniversalId::Type, RefIdDataContainerBase*>::const_iterator iter(mRecordContainers.begin());
         iter != mRecordContainers.end(); ++iter)
    {
        index += iter->second->getSize();

        if (type == iter->first)
            break;
    }

    return index;
}

void CSMWorld::RefIdData::load(ESM::ESMReader& reader, bool base, CSMWorld::UniversalId::Type type)
{
    std::map<UniversalId::Type, RefIdDataContainerBase*>::iterator found = mRecordContainers.find(type);

    if (found == mRecordContainers.end())
        throw std::logic_error("Invalid Referenceable ID type");

    int index = found->second->load(reader, base);
    if (index != -1)
    {
        LocalIndex localIndex = LocalIndex(index, type);
        if (base && getRecord(localIndex).mState == RecordBase::State_Deleted)
        {
            erase(localIndex, 1);
        }
        else
        {
            mIndex[getRecordId(localIndex)] = localIndex;
        }
    }
}

void CSMWorld::RefIdData::erase(const LocalIndex& index, int count)
{
    std::map<UniversalId::Type, RefIdDataContainerBase*>::iterator iter = mRecordContainers.find(index.second);
    if (iter == mRecordContainers.end())
        throw std::logic_error("invalid local index type");

    for (int i = index.first; i < index.first + count; ++i)
    {
        auto result = mIndex.find(iter->second->getId(i));

        if (result != mIndex.end())
            mIndex.erase(result);
    }

    // Adjust the local indexes to avoid gaps between them after removal of records
    int recordIndex = index.first + count;
    int recordCount = iter->second->getSize();
    while (recordIndex < recordCount)
    {
        auto recordIndexFound = mIndex.find(iter->second->getId(recordIndex));
        if (recordIndexFound != mIndex.end())
        {
            recordIndexFound->second.first -= count;
        }
        ++recordIndex;
    }

    iter->second->erase(index.first, count);
}

int CSMWorld::RefIdData::getSize() const
{
    return static_cast<int>(mIndex.size());
}

std::vector<ESM::RefId> CSMWorld::RefIdData::getIds(bool listDeleted) const
{
    std::vector<ESM::RefId> ids;

    for (auto iter(mIndex.begin()); iter != mIndex.end(); ++iter)
    {
        if (listDeleted || !getRecord(iter->second).isDeleted())
        {
            std::map<UniversalId::Type, RefIdDataContainerBase*>::const_iterator container
                = mRecordContainers.find(iter->second.second);

            if (container == mRecordContainers.end())
                throw std::logic_error("Invalid referenceable ID type");

            ids.push_back(container->second->getId(iter->second.first));
        }
    }

    return ids;
}

void CSMWorld::RefIdData::save(int index, ESM::ESMWriter& writer) const
{
    LocalIndex localIndex = globalToLocalIndex(index);

    std::map<UniversalId::Type, RefIdDataContainerBase*>::const_iterator iter
        = mRecordContainers.find(localIndex.second);

    if (iter == mRecordContainers.end())
        throw std::logic_error("invalid local index type");

    iter->second->save(localIndex.first, writer);
}

const CSMWorld::RefIdDataContainer<ESM::Book>& CSMWorld::RefIdData::getBooks() const
{
    return mBooks;
}

const CSMWorld::RefIdDataContainer<ESM::Activator>& CSMWorld::RefIdData::getActivators() const
{
    return mActivators;
}

const CSMWorld::RefIdDataContainer<ESM::Potion>& CSMWorld::RefIdData::getPotions() const
{
    return mPotions;
}

const CSMWorld::RefIdDataContainer<ESM::Apparatus>& CSMWorld::RefIdData::getApparati() const
{
    return mApparati;
}

const CSMWorld::RefIdDataContainer<ESM::Armor>& CSMWorld::RefIdData::getArmors() const
{
    return mArmors;
}

const CSMWorld::RefIdDataContainer<ESM::Clothing>& CSMWorld::RefIdData::getClothing() const
{
    return mClothing;
}

const CSMWorld::RefIdDataContainer<ESM::Container>& CSMWorld::RefIdData::getContainers() const
{
    return mContainers;
}

const CSMWorld::RefIdDataContainer<ESM::Creature>& CSMWorld::RefIdData::getCreatures() const
{
    return mCreatures;
}

const CSMWorld::RefIdDataContainer<ESM::Door>& CSMWorld::RefIdData::getDoors() const
{
    return mDoors;
}

const CSMWorld::RefIdDataContainer<ESM::Ingredient>& CSMWorld::RefIdData::getIngredients() const
{
    return mIngredients;
}

const CSMWorld::RefIdDataContainer<ESM::CreatureLevList>& CSMWorld::RefIdData::getCreatureLevelledLists() const
{
    return mCreatureLevelledLists;
}

const CSMWorld::RefIdDataContainer<ESM::ItemLevList>& CSMWorld::RefIdData::getItemLevelledList() const
{
    return mItemLevelledLists;
}

const CSMWorld::RefIdDataContainer<ESM::Light>& CSMWorld::RefIdData::getLights() const
{
    return mLights;
}

const CSMWorld::RefIdDataContainer<ESM::Lockpick>& CSMWorld::RefIdData::getLocpicks() const
{
    return mLockpicks;
}

const CSMWorld::RefIdDataContainer<ESM::Miscellaneous>& CSMWorld::RefIdData::getMiscellaneous() const
{
    return mMiscellaneous;
}

const CSMWorld::RefIdDataContainer<ESM::NPC>& CSMWorld::RefIdData::getNPCs() const
{
    return mNpcs;
}

const CSMWorld::RefIdDataContainer<ESM::Weapon>& CSMWorld::RefIdData::getWeapons() const
{
    return mWeapons;
}

const CSMWorld::RefIdDataContainer<ESM::Probe>& CSMWorld::RefIdData::getProbes() const
{
    return mProbes;
}

const CSMWorld::RefIdDataContainer<ESM::Repair>& CSMWorld::RefIdData::getRepairs() const
{
    return mRepairs;
}

const CSMWorld::RefIdDataContainer<ESM::Static>& CSMWorld::RefIdData::getStatics() const
{
    return mStatics;
}

void CSMWorld::RefIdData::insertRecord(
    std::unique_ptr<CSMWorld::RecordBase> record, CSMWorld::UniversalId::Type type, const ESM::RefId& id)
{
    std::map<UniversalId::Type, RefIdDataContainerBase*>::iterator iter = mRecordContainers.find(type);

    if (iter == mRecordContainers.end())
        throw std::logic_error("invalid local index type");

    iter->second->insertRecord(std::move(record));

    mIndex.insert(std::make_pair(id, LocalIndex(iter->second->getSize() - 1, type)));
}

void CSMWorld::RefIdData::copyTo(int index, RefIdData& target) const
{
    LocalIndex localIndex = globalToLocalIndex(index);

    auto foundIndex = mRecordContainers.find(localIndex.second);
    assert(foundIndex != mRecordContainers.end());
    RefIdDataContainerBase* source = foundIndex->second;

    target.insertRecord(
        source->getRecord(localIndex.first).modifiedCopy(), localIndex.second, source->getId(localIndex.first));
}
