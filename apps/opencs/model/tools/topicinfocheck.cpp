#include "topicinfocheck.hpp"

#include <sstream>

#include "../prefs/state.hpp"

#include "../world/infoselectwrapper.hpp"

CSMTools::TopicInfoCheckStage::TopicInfoCheckStage(
    const CSMWorld::InfoCollection& topicInfos,
    const CSMWorld::IdCollection<CSMWorld::Cell>& cells,
    const CSMWorld::IdCollection<ESM::Class>& classes,
    const CSMWorld::IdCollection<ESM::Faction>& factions,
    const CSMWorld::IdCollection<ESM::GameSetting>& gmsts,
    const CSMWorld::IdCollection<ESM::Global>& globals,
    const CSMWorld::IdCollection<ESM::Dialogue>& journals,
    const CSMWorld::IdCollection<ESM::Race>& races,
    const CSMWorld::IdCollection<ESM::Region>& regions,
    const CSMWorld::IdCollection<ESM::Dialogue> &topics,
    const CSMWorld::RefIdData& referencables,
    const CSMWorld::Resources& soundFiles)
    : mTopicInfos(topicInfos),
      mCells(cells),
      mClasses(classes),
      mFactions(factions),
      mGameSettings(gmsts),
      mGlobals(globals),
      mJournals(journals),
      mRaces(races),
      mRegions(regions),
      mTopics(topics),
      mReferencables(referencables),
      mSoundFiles(soundFiles)
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
    int index = mGameSettings.searchId("sDefaultCellname");
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
        verifyCell(topicInfo.mCell, id, messages);
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

    for (std::vector<ESM::DialInfo::SelectStruct>::const_iterator it = topicInfo.mSelects.begin();
         it != topicInfo.mSelects.end(); ++it)
    {
        verifySelectStruct((*it), id, messages);
    }
}

// Verification functions

bool CSMTools::TopicInfoCheckStage::verifyActor(const std::string& actor, const CSMWorld::UniversalId& id,
    CSMDoc::Messages& messages)
{
    CSMWorld::RefIdData::LocalIndex index = mReferencables.searchId(actor);

    if (index.first == -1)
    {
        messages.add(id, "Actor '" + actor + "' does not exist", "", CSMDoc::Message::Severity_Error);
        return false;
    }
    else if (mReferencables.getRecord(index).isDeleted())
    {
        messages.add(id, "Deleted actor '" + actor + "' is being referenced", "", CSMDoc::Message::Severity_Error);
        return false;
    }
    else if (index.second != CSMWorld::UniversalId::Type_Npc && index.second != CSMWorld::UniversalId::Type_Creature)
    {
        CSMWorld::UniversalId tempId(index.second, actor);
        std::ostringstream stream;
        stream << "Object '" << actor << "' has invalid type " << tempId.getTypeName() << " (an actor must be an NPC or a creature)"; 
        messages.add(id, stream.str(), "", CSMDoc::Message::Severity_Error);
        return false;
    }

    return true;
}

bool CSMTools::TopicInfoCheckStage::verifyCell(const std::string& cell, const CSMWorld::UniversalId& id,
    CSMDoc::Messages& messages)
{
    if (mCellNames.find(cell) == mCellNames.end())
    {
        messages.add(id, "Cell '" + cell + "' does not exist", "", CSMDoc::Message::Severity_Error);
        return false;
    }

    return true;
}

bool CSMTools::TopicInfoCheckStage::verifyFactionRank(const std::string& factionName, int rank, const CSMWorld::UniversalId& id,
    CSMDoc::Messages& messages)
{
    if (rank < -1)
    {
        std::ostringstream stream;
        stream << "Faction rank is set to " << rank << ", but it should be set to -1 if there are no rank requirements";
        messages.add(id, stream.str(), "", CSMDoc::Message::Severity_Warning);
        return false;
    }

    int index = mFactions.searchId(factionName);

    const ESM::Faction &faction = mFactions.getRecord(index).get();

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

bool CSMTools::TopicInfoCheckStage::verifyItem(const std::string& item, const CSMWorld::UniversalId& id,
    CSMDoc::Messages& messages)
{
    CSMWorld::RefIdData::LocalIndex index = mReferencables.searchId(item);

    if (index.first == -1)
    {
        messages.add(id, ("Item '" + item + "' does not exist"), "", CSMDoc::Message::Severity_Error);
        return false;
    }
    else if (mReferencables.getRecord(index).isDeleted())
    {
        messages.add(id, ("Deleted item '" + item + "' is being referenced"), "", CSMDoc::Message::Severity_Error);
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
                stream << "Object '" << item << "' has invalid type " << tempId.getTypeName() << " (an item can be a potion, an armor piece, a book and so on)"; 
                messages.add(id, stream.str(), "", CSMDoc::Message::Severity_Error);
                return false;
            }
        }
    }

    return true;
}

bool CSMTools::TopicInfoCheckStage::verifySelectStruct(const ESM::DialInfo::SelectStruct& select,
    const CSMWorld::UniversalId& id, CSMDoc::Messages& messages)
{
    CSMWorld::ConstInfoSelectWrapper infoCondition(select);

    if (infoCondition.getFunctionName() == CSMWorld::ConstInfoSelectWrapper::Function_None)
    {
        messages.add(id, "Invalid condition '" + infoCondition.toString() + "'", "", CSMDoc::Message::Severity_Error);
        return false;
    }
    else if (!infoCondition.variantTypeIsValid())
    {
        std::ostringstream stream;
        stream << "Value of condition '" << infoCondition.toString() << "' has invalid ";

        switch (select.mValue.getType())
        {
            case ESM::VT_None:   stream << "None"; break;
            case ESM::VT_Short:  stream << "Short"; break;
            case ESM::VT_Int:    stream << "Int"; break;
            case ESM::VT_Long:   stream << "Long"; break;
            case ESM::VT_Float:  stream << "Float"; break;
            case ESM::VT_String: stream << "String"; break;
            default:             stream << "unknown"; break;
        }
        stream << " type";

        messages.add(id, stream.str(), "", CSMDoc::Message::Severity_Error);
        return false;
    }
    else if (infoCondition.conditionIsAlwaysTrue())
    {
        messages.add(id, "Condition '" + infoCondition.toString() + "' is always true", "", CSMDoc::Message::Severity_Warning);
        return false;
    }
    else if (infoCondition.conditionIsNeverTrue())
    {
        messages.add(id, "Condition '" + infoCondition.toString() + "' is never true", "", CSMDoc::Message::Severity_Warning);
        return false;
    }

    // Id checks
    if (infoCondition.getFunctionName() == CSMWorld::ConstInfoSelectWrapper::Function_Global &&
        !verifyId(infoCondition.getVariableName(), mGlobals, id, messages))
    {
        return false;
    }
    else if (infoCondition.getFunctionName() == CSMWorld::ConstInfoSelectWrapper::Function_Journal &&
        !verifyId(infoCondition.getVariableName(), mJournals, id, messages))
    {
        return false;
    }
    else if (infoCondition.getFunctionName() == CSMWorld::ConstInfoSelectWrapper::Function_Item &&
        !verifyItem(infoCondition.getVariableName(), id, messages))
    {
        return false;
    }
    else if (infoCondition.getFunctionName() == CSMWorld::ConstInfoSelectWrapper::Function_Dead &&
        !verifyActor(infoCondition.getVariableName(), id, messages))
    {
        return false;
    }
    else if (infoCondition.getFunctionName() == CSMWorld::ConstInfoSelectWrapper::Function_NotId &&
        !verifyActor(infoCondition.getVariableName(), id, messages))
    {
        return false;
    }
    else if (infoCondition.getFunctionName() == CSMWorld::ConstInfoSelectWrapper::Function_NotFaction &&
        !verifyId(infoCondition.getVariableName(), mFactions, id, messages))
    {
        return false;
    }
    else if (infoCondition.getFunctionName() == CSMWorld::ConstInfoSelectWrapper::Function_NotClass &&
        !verifyId(infoCondition.getVariableName(), mClasses, id, messages))
    {
        return false;
    }
    else if (infoCondition.getFunctionName() == CSMWorld::ConstInfoSelectWrapper::Function_NotRace &&
        !verifyId(infoCondition.getVariableName(), mRaces, id, messages))
    {
        return false;
    }
    else if (infoCondition.getFunctionName() == CSMWorld::ConstInfoSelectWrapper::Function_NotCell &&
        !verifyCell(infoCondition.getVariableName(), id, messages))
    {
        return false;
    }

    return true;
}

bool CSMTools::TopicInfoCheckStage::verifySound(const std::string& sound, const CSMWorld::UniversalId& id,
    CSMDoc::Messages& messages)
{
    if (mSoundFiles.searchId(sound) == -1)
    {
        messages.add(id, "Sound file '" + sound + "' does not exist", "", CSMDoc::Message::Severity_Error);
        return false;
    }

    return true;
}

template <typename T>
bool CSMTools::TopicInfoCheckStage::verifyId(const std::string& name, const CSMWorld::IdCollection<T>& collection,
    const CSMWorld::UniversalId& id, CSMDoc::Messages& messages)
{
    int index = collection.searchId(name);

    if (index == -1)
    {
        messages.add(id, T::getRecordType() + " '" + name + "' does not exist", "", CSMDoc::Message::Severity_Error);
        return false;
    }
    else if (collection.getRecord(index).isDeleted())
    {
        messages.add(id, "Deleted " + T::getRecordType() + " record '" + name + "' is being referenced", "", CSMDoc::Message::Severity_Error);
        return false;
    }

    return true;
}
