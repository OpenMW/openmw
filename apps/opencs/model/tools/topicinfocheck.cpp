#include "topicinfocheck.hpp"

#include <sstream>

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
{}

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
    
    return mTopicInfos.getSize();
}

void CSMTools::TopicInfoCheckStage::perform(int stage, CSMDoc::Messages& messages)
{
    const CSMWorld::Record<CSMWorld::Info>& infoRecord = mTopicInfos.getRecord(stage);
    
    if (infoRecord.isDeleted())
        return;
    
    const CSMWorld::Info& topicInfo = infoRecord.get();
    
    // There should always be a topic that matches
    int topicIndex = mTopics.searchId(topicInfo.mTopicId);
    
    const CSMWorld::Record<ESM::Dialogue>& topicRecord = mTopics.getRecord(topicIndex);
    
    if (topicRecord.isDeleted())
        return;
    
    const ESM::Dialogue& topic = topicRecord.get();
    
    CSMWorld::UniversalId id(CSMWorld::UniversalId::Type_TopicInfo, topicInfo.mId);
    
    checkActor(topicInfo.mActor, id, messages);
    checkClass(topicInfo.mClass, id, messages);
    checkCell(topicInfo.mCell, id, messages);
    checkFaction(topicInfo.mFaction, topicInfo.mData.mRank, id, messages);
    checkFaction(topicInfo.mPcFaction, topicInfo.mData.mPCrank, id, messages);
    checkRace(topicInfo.mRace, id, messages);
    checkSound(topicInfo.mSound, id, messages);
    
    for (std::vector<ESM::DialInfo::SelectStruct>::const_iterator it = topicInfo.mSelects.begin();
         it != topicInfo.mSelects.end(); ++it)
    {
        checkSelectStruct((*it), id, messages);
    }
    
    if (topic.mType != ESM::Dialogue::Voice)
    {
        checkResponse(topicInfo.mResponse, id, messages);
    }
}

void CSMTools::TopicInfoCheckStage::checkActor(const std::string& actor, const CSMWorld::UniversalId& id,
        CSMDoc::Messages& messages)
{
    if (actor.empty())
        return;
    
    CSMWorld::RefIdData::LocalIndex index = mReferencables.searchId(actor);
    
    if (index.first == -1)
    {
        messages.add(id, actor + " is not a referencable object", "", CSMDoc::Message::Severity_Error);
    }
    else if (mReferencables.getRecord(index).isDeleted())
    {
        messages.add(id, actor + " is a deleted object", "", CSMDoc::Message::Severity_Error);
    }
    else if (index.second != CSMWorld::UniversalId::Type_Npc && index.second != CSMWorld::UniversalId::Type_Creature)
    {
        CSMWorld::UniversalId tempId(index.second, actor);

        std::ostringstream stream;
        stream << actor << " is of type " << tempId.getTypeName() << ", not of type Creature or NPC";
        
        messages.add(id, stream.str(), "", CSMDoc::Message::Severity_Error);
    }
}

void CSMTools::TopicInfoCheckStage::checkCell(const std::string& cellName, const CSMWorld::UniversalId& id, 
        CSMDoc::Messages& messages)
{
    if (cellName.empty())
        return;
    
    if (mCellNames.find(cellName) == mCellNames.end())
    {
        messages.add(id, cellName + " is not a cell name", "", CSMDoc::Message::Severity_Error);
    }
}

void CSMTools::TopicInfoCheckStage::checkClass(const std::string& className, const CSMWorld::UniversalId& id,
        CSMDoc::Messages& messages)
{
    if (className.empty())
        return;
    
    int index = mClasses.searchId(className);
    
    if (index == -1)
    {
        messages.add(id, className + " is not a class name", "", CSMDoc::Message::Severity_Error);
    }
    else if (mClasses.getRecord(index).isDeleted())
    {
        messages.add(id, className + " is a deleted record", "", CSMDoc::Message::Severity_Error);
    }
}

void CSMTools::TopicInfoCheckStage::checkFaction(const std::string& factionName, int rank,
        const CSMWorld::UniversalId& id, CSMDoc::Messages& messages)
{
    if (factionName.empty())
        return;
    
    int index = mFactions.searchId(factionName);
    
    if (index == -1)
    {
        messages.add(id, factionName + " is not a faction name", "", CSMDoc::Message::Severity_Error);
    }
    else if (mFactions.getRecord(index).isDeleted())
    {
        messages.add(id, factionName + " is a deleted record", "", CSMDoc::Message::Severity_Error);
    }
    else if (rank < -1)
    {
        messages.add(id, "Faction rank should be set to -1 if no rank is required",
                     "", CSMDoc::Message::Severity_Error);
    }
    else
    {
        const ESM::Faction &faction = mFactions.getRecord(index).get();
        
        // calculate rank limit
        int limit = 0;
        for (; limit < 10; ++limit)
        {
            if (faction.mRanks[limit].empty())
                break;
        }
        
        if (rank >= limit)
        {
            std::ostringstream stream;
            stream << factionName << " is limited to ranks below " << limit;
            
            messages.add(id, stream.str(), "", CSMDoc::Message::Severity_Error);
        }
    }
}

void CSMTools::TopicInfoCheckStage::checkGlobal(const std::string& globalName, const CSMWorld::UniversalId& id,
        CSMDoc::Messages& messages)
{
    if (globalName.empty())
        return;
    
    int index = mGlobals.searchId(globalName);
    
    if (index == -1)
    {
        messages.add(id, globalName + " is not the name of a Global Variable", "", CSMDoc::Message::Severity_Error);
    }
    else if (mGlobals.getRecord(index).isDeleted())
    {
        messages.add(id, globalName + " is a deleted record", "", CSMDoc::Message::Severity_Error);
    }
}

void CSMTools::TopicInfoCheckStage::checkItem(const std::string& itemName, const CSMWorld::UniversalId& id,
        CSMDoc::Messages& messages)
{
    CSMWorld::RefIdData::LocalIndex index = mReferencables.searchId(itemName);

    if (index.first == -1)
    {
        messages.add(id, itemName + " is not a referencable object", "", CSMDoc::Message::Severity_Error);
    }
    else if (mReferencables.getRecord(index).isDeleted())
    {
        messages.add(id, itemName + " is a deleted object", "", CSMDoc::Message::Severity_Error);
    }
    else
    {
        CSMWorld::UniversalId tempId(index.second, itemName);
        std::ostringstream stream;
        
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
                stream << itemName << " is of type " << tempId.getTypeName() << ", which is not an item type";
                messages.add(id, stream.str(), "", CSMDoc::Message::Severity_Error);
                break;
        }
    }
}

void CSMTools::TopicInfoCheckStage::checkJournal(const std::string& journalName, const CSMWorld::UniversalId& id,
        CSMDoc::Messages& messages)
{
    if (journalName.empty())
        return;
    
    int index = mJournals.searchId(journalName);
    
    if (index == -1)
    {
        messages.add(id, journalName + " is not the name of a Journal Topic", "", CSMDoc::Message::Severity_Error);
    }
    else if (mJournals.getRecord(index).isDeleted())
    {
        messages.add(id, journalName + " is a deleted record", "", CSMDoc::Message::Severity_Error);
    }
}

void CSMTools::TopicInfoCheckStage::checkRace(const std::string& race, const CSMWorld::UniversalId& id,
        CSMDoc::Messages& messages)
{
    if (race.empty())
        return;
    
    int index = mRaces.searchId(race);
    
    if (index == -1)
    {
        messages.add(id, race + " is not the name of a race", "", CSMDoc::Message::Severity_Error);
    }
    else if (mRaces.getRecord(index).isDeleted())
    {
        messages.add(id, race + " is a deleted record", "", CSMDoc::Message::Severity_Error);
    }
}

void CSMTools::TopicInfoCheckStage::checkResponse(const std::string& response, const CSMWorld::UniversalId& id, 
        CSMDoc::Messages& messages)
{
    if (response.empty())
    {
        messages.add(id, "Missing response", "", CSMDoc::Message::Severity_Error);
    }
}

// ESM::DialInfo::SelectStruct.mSelectRule
// 012345...
// ^^^ ^^
// ||| ||
// ||| |+------------- condition variable string
// ||| +-------------- comparison type, ['0'..'5']; e.g. !=, <, >=, etc
// ||+---------------- function index (encoded, where function == '1')
// |+----------------- function, ['1'..'C']; e.g. Global, Local, Not ID, etc
// +------------------ unknown
void CSMTools::TopicInfoCheckStage::checkSelectStruct(const ESM::DialInfo::SelectStruct& selectStruct, 
        const CSMWorld::UniversalId& id, CSMDoc::Messages& messages)
{
    std::string varName = selectStruct.mSelectRule.substr(5);
    
    switch (selectStruct.mSelectRule[1])
    {
        case '0': // blank space
            messages.add(id, "Info Condition is empty", "", CSMDoc::Message::Severity_Warning);
            break;
            
        case '1': // Function
            break;
            
        case '2': // Global
            checkGlobal(varName, id, messages);
            break;
            
        case '3': // Local
            break;
            
        case '4': // Journal
            checkJournal(varName, id, messages);
            break;
            
        case '5': // Item
            checkItem(varName, id, messages);
            break;
            
        case '6': // Dead
            checkActor(varName, id, messages);
            break;
            
        case '7': // Not ID
            checkActor(varName, id, messages);
            break;
            
        case '8': // Not Faction
            checkFaction(varName, -1, id, messages);
            break;
            
        case '9': // Not Class
            checkClass(varName, id, messages);
            break;
            
        case 'A': // Not Race
            checkRace(varName, id, messages);
            break;
            
        case 'B': // Not Cell
            checkCell(varName, id, messages);
            break;
            
        case 'C': // Not Local
            break;
    }
}

void CSMTools::TopicInfoCheckStage::checkSound(const std::string& sound, const CSMWorld::UniversalId& id,
        CSMDoc::Messages& messages)
{
    if (sound.empty())
        return;
    
    if (mSoundFiles.searchId(sound) == -1)
    {
        messages.add(id, sound + " is not a known sound file", "", CSMDoc::Message::Severity_Error);
    }
}
