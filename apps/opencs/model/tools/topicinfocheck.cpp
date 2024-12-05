#include "topicinfocheck.hpp"

#include <sstream>
#include <string>
#include <vector>

#include <apps/opencs/model/doc/messages.hpp>
#include <apps/opencs/model/prefs/category.hpp>
#include <apps/opencs/model/prefs/setting.hpp>
#include <apps/opencs/model/world/cell.hpp>
#include <apps/opencs/model/world/idcollection.hpp>
#include <apps/opencs/model/world/info.hpp>
#include <apps/opencs/model/world/infocollection.hpp>
#include <apps/opencs/model/world/record.hpp>
#include <apps/opencs/model/world/refiddata.hpp>
#include <apps/opencs/model/world/resources.hpp>
#include <apps/opencs/model/world/universalid.hpp>

#include <components/esm3/loadclas.hpp>
#include <components/esm3/loaddial.hpp>
#include <components/esm3/loadfact.hpp>
#include <components/esm3/loadglob.hpp>
#include <components/esm3/loadgmst.hpp>
#include <components/esm3/loadinfo.hpp>
#include <components/esm3/loadrace.hpp>
#include <components/esm3/loadregn.hpp>
#include <components/esm3/variant.hpp>

#include "../prefs/state.hpp"

#include "../world/infoselectwrapper.hpp"

CSMTools::TopicInfoCheckStage::TopicInfoCheckStage(const CSMWorld::InfoCollection& topicInfos,
    const CSMWorld::IdCollection<CSMWorld::Cell>& cells, const CSMWorld::IdCollection<ESM::Class>& classes,
    const CSMWorld::IdCollection<ESM::Faction>& factions, const CSMWorld::IdCollection<ESM::GameSetting>& gmsts,
    const CSMWorld::IdCollection<ESM::Global>& globals, const CSMWorld::IdCollection<ESM::Dialogue>& journals,
    const CSMWorld::IdCollection<ESM::Race>& races, const CSMWorld::IdCollection<ESM::Region>& regions,
    const CSMWorld::IdCollection<ESM::Dialogue>& topics, const CSMWorld::RefIdData& referencables,
    const CSMWorld::Resources& soundFiles)
    : mTopicInfos(topicInfos)
    , mCells(cells)
    , mClasses(classes)
    , mFactions(factions)
    , mGameSettings(gmsts)
    , mGlobals(globals)
    , mJournals(journals)
    , mRaces(races)
    , mRegions(regions)
    , mTopics(topics)
    , mReferencables(referencables)
    , mSoundFiles(soundFiles)
{
    mIgnoreBaseRecords = false;
}

int CSMTools::TopicInfoCheckStage::setup()
{
    // Generate list of cell names for reference checking

    mCellNames.clear();
    for (int i = 0; i < mCells.getSize(); ++i)
    {
        const CSMWorld::Record<CSMWorld::Cell>& cellRecord = mCells.getRecord(i);

        if (cellRecord.isDeleted())
            continue;

        mCellNames.insert(cellRecord.get().mName);
    }
    // Cell names can also include region names
    for (int i = 0; i < mRegions.getSize(); ++i)
    {
        const CSMWorld::Record<ESM::Region>& regionRecord = mRegions.getRecord(i);

        if (regionRecord.isDeleted())
            continue;

        mCellNames.insert(regionRecord.get().mName);
    }
    // Default cell name
    const int index = mGameSettings.searchId(ESM::RefId::stringRefId("sDefaultCellname"));
    if (index != -1)
    {
        const CSMWorld::Record<ESM::GameSetting>& gmstRecord = mGameSettings.getRecord(index);

        if (!gmstRecord.isDeleted() && gmstRecord.get().mValue.getType() == ESM::VT_String)
        {
            mCellNames.insert(gmstRecord.get().mValue.getString());
        }
    }

    mIgnoreBaseRecords = CSMPrefs::get()["Reports"]["ignore-base-records"].isTrue();

    return mTopicInfos.getSize();
}

void CSMTools::TopicInfoCheckStage::perform(int stage, CSMDoc::Messages& messages)
{
    const CSMWorld::Record<CSMWorld::Info>& infoRecord = mTopicInfos.getRecord(stage);

    // Skip "Base" records (setting!) and "Deleted" records
    if ((mIgnoreBaseRecords && infoRecord.mState == CSMWorld::RecordBase::State_BaseOnly) || infoRecord.isDeleted())
        return;

    const CSMWorld::Info& topicInfo = infoRecord.get();

    // There should always be a topic that matches
    int topicIndex = mTopics.searchId(topicInfo.mTopicId);

    const CSMWorld::Record<ESM::Dialogue>& topicRecord = mTopics.getRecord(topicIndex);

    if (topicRecord.isDeleted())
        return;

    const ESM::Dialogue& topic = topicRecord.get();

    CSMWorld::UniversalId id(CSMWorld::UniversalId::Type_TopicInfo, topicInfo.mId);

    // Check fields

    if (!topicInfo.mActor.empty())
    {
        verifyActor(topicInfo.mActor, id, messages);
    }

    if (!topicInfo.mClass.empty())
    {
        verifyId(topicInfo.mClass, mClasses, id, messages);
    }

    if (!topicInfo.mCell.empty())
    {
        verifyCell(topicInfo.mCell.getRefIdString(), id, messages);
    }

    if (!topicInfo.mFaction.empty() && !topicInfo.mFactionLess)
    {
        if (verifyId(topicInfo.mFaction, mFactions, id, messages))
        {
            verifyFactionRank(topicInfo.mFaction, topicInfo.mData.mRank, id, messages);
        }
    }

    if (!topicInfo.mPcFaction.empty())
    {
        if (verifyId(topicInfo.mPcFaction, mFactions, id, messages))
        {
            verifyFactionRank(topicInfo.mPcFaction, topicInfo.mData.mPCrank, id, messages);
        }
    }

    if (topicInfo.mData.mGender < -1 || topicInfo.mData.mGender > 1)
    {
        messages.add(id, "Gender is invalid", "", CSMDoc::Message::Severity_Error);
    }

    if (!topicInfo.mRace.empty())
    {
        verifyId(topicInfo.mRace, mRaces, id, messages);
    }

    if (!topicInfo.mSound.empty())
    {
        verifySound(topicInfo.mSound, id, messages);
    }

    if (topicInfo.mResponse.empty() && topic.mType != ESM::Dialogue::Voice)
    {
        messages.add(id, "Response is empty", "", CSMDoc::Message::Severity_Warning);
    }

    // Check info conditions

    for (const auto& select : topicInfo.mSelects)
    {
        verifySelectStruct(select, id, messages);
    }
}

// Verification functions

bool CSMTools::TopicInfoCheckStage::verifyActor(
    const ESM::RefId& actor, const CSMWorld::UniversalId& id, CSMDoc::Messages& messages)
{
    const std::string& actorString = actor.getRefIdString();
    CSMWorld::RefIdData::LocalIndex index = mReferencables.searchId(actor);

    if (index.first == -1)
    {
        messages.add(id, "Actor '" + actorString + "' does not exist", "", CSMDoc::Message::Severity_Error);
        return false;
    }
    else if (mReferencables.getRecord(index).isDeleted())
    {
        messages.add(
            id, "Deleted actor '" + actorString + "' is being referenced", "", CSMDoc::Message::Severity_Error);
        return false;
    }
    else if (index.second != CSMWorld::UniversalId::Type_Npc && index.second != CSMWorld::UniversalId::Type_Creature)
    {
        CSMWorld::UniversalId tempId(index.second, actor);
        std::ostringstream stream;
        stream << "Object '" << actor << "' has invalid type " << tempId.getTypeName()
               << " (an actor must be an NPC or a creature)";
        messages.add(id, stream.str(), "", CSMDoc::Message::Severity_Error);
        return false;
    }

    return true;
}

bool CSMTools::TopicInfoCheckStage::verifyCell(
    const std::string& cell, const CSMWorld::UniversalId& id, CSMDoc::Messages& messages)
{
    if (mCellNames.find(cell) == mCellNames.end())
    {
        messages.add(id, "Cell '" + cell + "' does not exist", "", CSMDoc::Message::Severity_Error);
        return false;
    }

    return true;
}

bool CSMTools::TopicInfoCheckStage::verifyFactionRank(
    const ESM::RefId& factionName, int rank, const CSMWorld::UniversalId& id, CSMDoc::Messages& messages)
{
    if (rank < -1)
    {
        std::ostringstream stream;
        stream << "Faction rank is set to " << rank << ", but it should be set to -1 if there are no rank requirements";
        messages.add(id, stream.str(), "", CSMDoc::Message::Severity_Warning);
        return false;
    }

    int index = mFactions.searchId(factionName);

    const ESM::Faction& faction = mFactions.getRecord(index).get();

    int limit = 0;
    for (; limit < 10; ++limit)
    {
        if (faction.mRanks[limit].empty())
            break;
    }

    if (rank >= limit)
    {
        std::ostringstream stream;
        stream << "Faction rank is set to " << rank << " which is more than the maximum of " << limit - 1
               << " for the '" << factionName << "' faction";

        messages.add(id, stream.str(), "", CSMDoc::Message::Severity_Error);
        return false;
    }

    return true;
}

bool CSMTools::TopicInfoCheckStage::verifyItem(
    const ESM::RefId& item, const CSMWorld::UniversalId& id, CSMDoc::Messages& messages)
{
    const std::string& idString = item.getRefIdString();
    CSMWorld::RefIdData::LocalIndex index = mReferencables.searchId(item);

    if (index.first == -1)
    {
        messages.add(id, ("Item '" + idString + "' does not exist"), "", CSMDoc::Message::Severity_Error);
        return false;
    }
    else if (mReferencables.getRecord(index).isDeleted())
    {
        messages.add(id, ("Deleted item '" + idString + "' is being referenced"), "", CSMDoc::Message::Severity_Error);
        return false;
    }
    else
    {
        switch (index.second)
        {
            case CSMWorld::UniversalId::Type_Potion:
            case CSMWorld::UniversalId::Type_Apparatus:
            case CSMWorld::UniversalId::Type_Armor:
            case CSMWorld::UniversalId::Type_Book:
            case CSMWorld::UniversalId::Type_Clothing:
            case CSMWorld::UniversalId::Type_Ingredient:
            case CSMWorld::UniversalId::Type_Light:
            case CSMWorld::UniversalId::Type_Lockpick:
            case CSMWorld::UniversalId::Type_Miscellaneous:
            case CSMWorld::UniversalId::Type_Probe:
            case CSMWorld::UniversalId::Type_Repair:
            case CSMWorld::UniversalId::Type_Weapon:
            case CSMWorld::UniversalId::Type_ItemLevelledList:
                break;

            default:
            {
                CSMWorld::UniversalId tempId(index.second, item);
                std::ostringstream stream;
                stream << "Object '" << item << "' has invalid type " << tempId.getTypeName()
                       << " (an item can be a potion, an armor piece, a book and so on)";
                messages.add(id, stream.str(), "", CSMDoc::Message::Severity_Error);
                return false;
            }
        }
    }

    return true;
}

bool CSMTools::TopicInfoCheckStage::verifySelectStruct(
    const ESM::DialogueCondition& select, const CSMWorld::UniversalId& id, CSMDoc::Messages& messages)
{
    CSMWorld::ConstInfoSelectWrapper infoCondition(select);

    if (select.mFunction == ESM::DialogueCondition::Function_None)
    {
        messages.add(id, "Invalid condition '" + infoCondition.toString() + "'", "", CSMDoc::Message::Severity_Error);
        return false;
    }
    else if (infoCondition.conditionIsAlwaysTrue())
    {
        messages.add(
            id, "Condition '" + infoCondition.toString() + "' is always true", "", CSMDoc::Message::Severity_Warning);
        return false;
    }
    else if (infoCondition.conditionIsNeverTrue())
    {
        messages.add(
            id, "Condition '" + infoCondition.toString() + "' is never true", "", CSMDoc::Message::Severity_Warning);
        return false;
    }

    // Id checks
    if (select.mFunction == ESM::DialogueCondition::Function_Global
        && !verifyId(ESM::RefId::stringRefId(select.mVariable), mGlobals, id, messages))
    {
        return false;
    }
    else if (select.mFunction == ESM::DialogueCondition::Function_Journal
        && !verifyId(ESM::RefId::stringRefId(select.mVariable), mJournals, id, messages))
    {
        return false;
    }
    else if (select.mFunction == ESM::DialogueCondition::Function_Item
        && !verifyItem(ESM::RefId::stringRefId(select.mVariable), id, messages))
    {
        return false;
    }
    else if (select.mFunction == ESM::DialogueCondition::Function_Dead
        && !verifyActor(ESM::RefId::stringRefId(select.mVariable), id, messages))
    {
        return false;
    }
    else if (select.mFunction == ESM::DialogueCondition::Function_NotId
        && !verifyActor(ESM::RefId::stringRefId(select.mVariable), id, messages))
    {
        return false;
    }
    else if (select.mFunction == ESM::DialogueCondition::Function_NotFaction
        && !verifyId(ESM::RefId::stringRefId(select.mVariable), mFactions, id, messages))
    {
        return false;
    }
    else if (select.mFunction == ESM::DialogueCondition::Function_NotClass
        && !verifyId(ESM::RefId::stringRefId(select.mVariable), mClasses, id, messages))
    {
        return false;
    }
    else if (select.mFunction == ESM::DialogueCondition::Function_NotRace
        && !verifyId(ESM::RefId::stringRefId(select.mVariable), mRaces, id, messages))
    {
        return false;
    }
    else if (select.mFunction == ESM::DialogueCondition::Function_NotCell
        && !verifyCell(select.mVariable, id, messages))
    {
        return false;
    }

    return true;
}

bool CSMTools::TopicInfoCheckStage::verifySound(
    const std::string& sound, const CSMWorld::UniversalId& id, CSMDoc::Messages& messages)
{
    if (mSoundFiles.searchId(sound) == -1)
    {
        messages.add(id, "Sound file '" + sound + "' does not exist", "", CSMDoc::Message::Severity_Error);
        return false;
    }

    return true;
}

template <typename T>
bool CSMTools::TopicInfoCheckStage::verifyId(const ESM::RefId& name, const CSMWorld::IdCollection<T>& collection,
    const CSMWorld::UniversalId& id, CSMDoc::Messages& messages)
{
    int index = collection.searchId(name);

    if (index == -1)
    {
        messages.add(id, std::string(T::getRecordType()) + " '" + name.getRefIdString() + "' does not exist", "",
            CSMDoc::Message::Severity_Error);
        return false;
    }
    else if (collection.getRecord(index).isDeleted())
    {
        messages.add(id,
            "Deleted " + std::string(T::getRecordType()) + " record '" + name.getRefIdString()
                + "' is being referenced",
            "", CSMDoc::Message::Severity_Error);
        return false;
    }

    return true;
}
