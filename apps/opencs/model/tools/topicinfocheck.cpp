#include "topicinfocheck.hpp"

#include <limits>
#include <sstream>

const int IMax = std::numeric_limits<int>::max();
const int IMin = std::numeric_limits<int>::min();

CSMTools::TopicInfoCheckStage::TopicInfoCheckStage(
    const CSMWorld::InfoCollection& topicInfos,
    const CSMWorld::InfoCollection& journalInfos,
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
      mJournalInfos(journalInfos),
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

bool CSMTools::TopicInfoCheckStage::checkActor(const std::string& actor, const CSMWorld::UniversalId& id,
        CSMDoc::Messages& messages)
{
    if (actor.empty())
        return false;
    
    CSMWorld::RefIdData::LocalIndex index = mReferencables.searchId(actor);
    
    if (index.first == -1)
    {
        messages.add(id, actor + " is not a referencable object", "", CSMDoc::Message::Severity_Error);
        return false;
    }
    else if (mReferencables.getRecord(index).isDeleted())
    {
        messages.add(id, actor + " is a deleted object", "", CSMDoc::Message::Severity_Error);
        return false;
    }
    else if (index.second != CSMWorld::UniversalId::Type_Npc && index.second != CSMWorld::UniversalId::Type_Creature)
    {
        CSMWorld::UniversalId tempId(index.second, actor);

        std::ostringstream stream;
        stream << actor << " is of type " << tempId.getTypeName() << ", not of type Creature or NPC";
        
        messages.add(id, stream.str(), "", CSMDoc::Message::Severity_Error);
        return false;
    }
    else
    {
        return true;
    }
}

bool CSMTools::TopicInfoCheckStage::checkCell(const std::string& cellName, const CSMWorld::UniversalId& id, 
        CSMDoc::Messages& messages)
{
    if (cellName.empty())
        return false;
    
    if (mCellNames.find(cellName) == mCellNames.end())
    {
        messages.add(id, cellName + " is not a cell name", "", CSMDoc::Message::Severity_Error);
        return false;
    }
    else
    {
        return true;
    }
}

bool CSMTools::TopicInfoCheckStage::checkClass(const std::string& className, const CSMWorld::UniversalId& id,
        CSMDoc::Messages& messages)
{
    if (className.empty())
        return false;
    
    int index = mClasses.searchId(className);
    
    if (index == -1)
    {
        messages.add(id, className + " is not a class name", "", CSMDoc::Message::Severity_Error);
        return false;
    }
    else if (mClasses.getRecord(index).isDeleted())
    {
        messages.add(id, className + " is a deleted record", "", CSMDoc::Message::Severity_Error);
        return false;
    }
    else
    {
        return true;
    }
}

bool CSMTools::TopicInfoCheckStage::checkFaction(const std::string& factionName, int rank,
        const CSMWorld::UniversalId& id, CSMDoc::Messages& messages)
{
    if (factionName.empty())
        return false;
    
    int index = mFactions.searchId(factionName);
    
    if (index == -1)
    {
        messages.add(id, factionName + " is not a faction name", "", CSMDoc::Message::Severity_Error);
        return false;
    }
    else if (mFactions.getRecord(index).isDeleted())
    {
        messages.add(id, factionName + " is a deleted record", "", CSMDoc::Message::Severity_Error);
        return false;
    }
    else if (rank < -1)
    {
        messages.add(id, "Faction rank should be set to -1 if no rank is required",
                     "", CSMDoc::Message::Severity_Error);
        return false;
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
            return false;
        }
        
        return true;
    }
    
}

bool CSMTools::TopicInfoCheckStage::checkGlobal(const std::string& globalName, const CSMWorld::UniversalId& id,
        CSMDoc::Messages& messages)
{
    if (globalName.empty())
        return false;
    
    int index = mGlobals.searchId(globalName);
    
    if (index == -1)
    {
        messages.add(id, globalName + " is not the name of a Global Variable", "", CSMDoc::Message::Severity_Error);
        return false;
    }
    else if (mGlobals.getRecord(index).isDeleted())
    {
        messages.add(id, globalName + " is a deleted record", "", CSMDoc::Message::Severity_Error);
        return false;
    }
    else
    {
        return true;
    }
}

bool CSMTools::TopicInfoCheckStage::checkItem(const std::string& itemName, const CSMWorld::UniversalId& id,
        CSMDoc::Messages& messages)
{
    if (itemName.empty())
        return false;
    
    CSMWorld::RefIdData::LocalIndex index = mReferencables.searchId(itemName);

    if (index.first == -1)
    {
        messages.add(id, itemName + " is not a referencable object", "", CSMDoc::Message::Severity_Error);
        return false;
    }
    else if (mReferencables.getRecord(index).isDeleted())
    {
        messages.add(id, itemName + " is a deleted object", "", CSMDoc::Message::Severity_Error);
        return false;
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
                return false;
        }
        
        return true;
    }
}

bool CSMTools::TopicInfoCheckStage::checkJournal(const std::string& journalName, const CSMWorld::UniversalId& id,
        CSMDoc::Messages& messages)
{
    if (journalName.empty())
        return false;
    
    int index = mJournals.searchId(journalName);
    
    if (index == -1)
    {
        messages.add(id, journalName + " is not the name of a Journal Topic", "", CSMDoc::Message::Severity_Error);
        return false;
    }
    else if (mJournals.getRecord(index).isDeleted())
    {
        messages.add(id, journalName + " is a deleted record", "", CSMDoc::Message::Severity_Error);
        return false;
    }
    else
    {
        return true;
    }
}

bool CSMTools::TopicInfoCheckStage::checkRace(const std::string& race, const CSMWorld::UniversalId& id,
        CSMDoc::Messages& messages)
{
    if (race.empty())
        return false;
    
    int index = mRaces.searchId(race);
    
    if (index == -1)
    {
        messages.add(id, race + " is not the name of a race", "", CSMDoc::Message::Severity_Error);
        return false;
    }
    else if (mRaces.getRecord(index).isDeleted())
    {
        messages.add(id, race + " is a deleted record", "", CSMDoc::Message::Severity_Error);
        return false;
    }
    else
    {
        return true;
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
    char functionName = selectStruct.mSelectRule[1];
    std::string varName = selectStruct.mSelectRule.substr(5);
    std::string functionIndex = selectStruct.mSelectRule.substr(2,2);
    char opName = selectStruct.mSelectRule[4];
    
    switch (functionName)
    {
        case '0': // blank space
            messages.add(id, "Info Condition is empty", "", CSMDoc::Message::Severity_Warning);
            break;
            
        case '1': // Function
            checkFunctionRange(functionIndex, opName, selectStruct.mValue, id, messages);
            break;
            
        case '2': // Global
            checkGlobal(varName, id, messages);
            break;
            
        case '3': // Local
            break;
            
        case '4': // Journal
            if (checkJournal(varName, id, messages))
            {
                checkJournalRange(varName, opName, selectStruct.mValue, id, messages);
            }
            
            break;
            
        case '5': // Item
            if (checkItem(varName, id, messages))
            {
                testIntRange(std::pair<int,int>(0, IMax), opName, selectStruct.mValue, "Item", id, messages);
            }
            break;
            
        case '6': // Dead
            if (checkActor(varName, id, messages))
            {
                testIntRange(std::pair<int,int>(0, IMax), opName, selectStruct.mValue, "Dead", id, messages);   
            }
            break;
            
        case '7': // Not ID
            if (checkActor(varName, id, messages))
            {
                testIntRange(std::pair<int,int>(0, 1), opName, selectStruct.mValue, "Not ID", id, messages);   
            }
            break;
            
        case '8': // Not Faction
            if (checkFaction(varName, -1, id, messages))
            {
                testIntRange(std::pair<int,int>(0, 1), opName, selectStruct.mValue, "Not Faction", id, messages);
            }
            break;
            
        case '9': // Not Class
            if (checkClass(varName, id, messages))
            {
                testIntRange(std::pair<int,int>(0, 1), opName, selectStruct.mValue, "Not Class", id, messages);   
            }
            break;
            
        case 'A': // Not Race
            if (checkRace(varName, id, messages))
            {
                testIntRange(std::pair<int,int>(0, 1), opName, selectStruct.mValue, "Not Race", id, messages);   
            }
            break;
            
        case 'B': // Not Cell
            if (checkCell(varName, id, messages))
            {
                testIntRange(std::pair<int,int>(0, 1), opName, selectStruct.mValue, "Not Cell", id, messages);            
            }
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

void CSMTools::TopicInfoCheckStage::checkFunctionRange(const std::string& functionIndex, char op, const ESM::Variant& variant,
        const CSMWorld::UniversalId& id, CSMDoc::Messages& messages)
{
    int functionCode = -1;
    std::stringstream stream;
    
    stream << functionIndex;
    stream >> functionCode;
    
    switch (functionCode)
    {
        case 0: // Rank Low
        case 1: // Rank High
        case 2: // Rank Requirement 
        case 3: // Reputation
        case 4: // Health Percent
        case 5: // PC Reputation
        case 6: // PC Level
        case 7: // PC Health Percent
        case 8: // PC Magicka
        case 9: // PC Fatigue
        case 10: // PC Strength
        case 11: // PC Block
        case 12: // PC Armorer
        case 13: // PC Medium Armor
        case 14: // PC Heavy Armor
        case 15: // PC Blunt Weapon
        case 16: // PC Long Blade
        case 17: // PC Axe
        case 18: // PC Spear
        case 19: // PC Athletics
        case 20: // PC Enchant
        case 21: // PC Destruction
        case 22: // PC Alteration
        case 23: // PC Illusion
        case 24: // PC Conjuration
        case 25: // PC Mysticism
        case 26: // PC Restoration
        case 27: // PC Alchemy
        case 28: // PC Unarmored
        case 29: // PC Security
        case 30: // PC Sneak
        case 31: // PC Acrobatics
        case 32: // PC Light Armor
        case 33: // PC Short Blade
        case 34: // PC Marksman
        case 35: // PC Merchantile
        case 36: // PC Speechcraft
        case 37: // PC Hand To Hand
        case 43: // PC Crime Level
        case 50: // Choice
        case 51: // PC Intelligence
        case 52: // PC Willpower
        case 53: // PC Agility
        case 54: // PC Speed
        case 55: // PC Endurance
        case 56: // PC Personality
        case 57: // PC Luck
        case 58: // PC Corpus
        case 59: // Weather
        case 61: // Level
        case 64: // PC Health
        case 66: // Friend Hit
        case 73: // PC Werewolf Kills
            
            testIntRange(std::pair<int,int>(0, IMax), op, variant, getFunctionName(functionCode), id, messages);
            break;
            
        case 38: // PC Sex
        case 39: // PC Expelled
        case 40: // PC Common Disease
        case 41: // PC Blight Disease
        case 44: // Same Sex
        case 45: // Same Race
        case 46: // Same Faction
        case 48: // Detected
        case 49: // Alarmed
        case 60: // PC Vampire
        case 62: // Attacked
        case 63: // Talked To PC
        case 67: // Fight
        case 68: // Hello
        case 69: // Alarm
        case 70: // Flee
        case 71: // Should Attack
        case 72: // Werewolf
            
            testIntRange(std::pair<int,int>(-1, 1), op, variant, getFunctionName(functionCode), id, messages);
            break;
            
        case 47: // Faction Rank Difference
            
            testIntRange(std::pair<int,int>(-9, 9), op, variant, getFunctionName(functionCode), id, messages);
            break;
            
        case 42: // PC Clothing Modifier
        case 65: // Creature Target
            break;
            
        default:
            messages.add(id, "An unknown Function Info Condition was found",
                         "", CSMDoc::Message::Severity_SeriousError);
            
    }
}

void CSMTools::TopicInfoCheckStage::checkJournalRange(const std::string& journalName, char op,
        const ESM::Variant& variant, const CSMWorld::UniversalId& id, CSMDoc::Messages& messages)
{
    if (variant.getType() != ESM::VT_Int && variant.getType() != ESM::VT_Short && variant.getType() != ESM::VT_Long)
    {
        messages.add(id, "Expected an integer type for Info Condition: Journal " + journalName,
                     "", CSMDoc::Message::Severity_Error);
    }
    
    // Journal is known to be valid
    int index = mJournals.searchId(journalName);
    const ESM::Dialogue &journal = mJournals.getRecord(index).get();
    
    std::pair<int,int> rangeReq1 = getRange(op, variant.getInteger(), true, id, messages);
    std::pair<int,int> rangeReq2 = getRange(op, variant.getInteger(), false, id, messages);
    
    CSMWorld::InfoCollection::Range journalRange = mJournalInfos.getTopicRange(journal.mId);
    
    for (CSMWorld::InfoCollection::RecordConstIterator it = journalRange.first; it != journalRange.second; ++it)
    {
        const CSMWorld::Record<CSMWorld::Info> infoRecord = (*it);
        
        if (infoRecord.isDeleted())
            continue;
        
        const CSMWorld::Info& journalInfo = infoRecord.get();
        
        index = journalInfo.mData.mJournalIndex;
        
        if (rangeIntersects(std::pair<int,int>(index, index), rangeReq1) ||
            rangeIntersects(std::pair<int,int>(index, index), rangeReq2))
        {
            // Found a journal index that fits requirements
            return;
        }
    }
    
    // It appears that journal index of 0 does not have to exist to meet requirements
    if (!rangeIntersects(std::pair<int,int>(0, 0), rangeReq1) &&
        !rangeIntersects(std::pair<int,int>(0, 0), rangeReq2))
    {
        // No journal index found that meets requirements
        messages.add(id, "Unattainable Info Condition: Journal " + journalName, "", CSMDoc::Message::Severity_Error);
    }
}

std::pair<int,int> CSMTools::TopicInfoCheckStage::getRange(char op, int value, bool lower,
        const CSMWorld::UniversalId& id, CSMDoc::Messages& messages)
{
    switch (op)
    {
        case '0': // =
            return std::pair<int,int>(value,value);
            
        case '1': // !=
            if (lower)
            {
                // Potential overflow
                if (value == IMax)
                {
                    return getRange(op,value,false, id, messages); // return upper range
                }
                else
                {
                    return std::pair<int,int>(IMin, value - 1);
                }
            }
            else // upper
            {
                // Potential overflow
                if (value == IMax)
                {
                    return getRange(op,value,true, id, messages); // return lower range
                }
                else
                {
                    return std::pair<int,int>(value + 1, IMax);
                }
            }
            
        case '2': // >
            // Potential overflow
            if (value == IMax)
            {
                return std::pair<int,int>(IMax, IMin);
            }
            else
            {
                return std::pair<int,int>(value + 1, IMax);
            }
            
        case '3': // >=
            return std::pair<int,int>(value, IMax);
            
        case '4': // <
            // Potential overflow
            if (value == IMin)
            {
                return std::pair<int,int>(IMax, IMin);
            }
            else
            {
                return std::pair<int,int>(IMin, value - 1);
            }
            
        case '5': // <=
            return std::pair<int,int>(IMin, value);
            
        default:
            messages.add(id, "Info condition has invalid operator", "", CSMDoc::Message::Severity_SeriousError);
            return std::pair<int,int>(IMin, IMax);
    }
}

bool CSMTools::TopicInfoCheckStage::rangeIntersects(std::pair<int,int> r1, std::pair<int,int> r2)
{
    return (r1.first <= r2.first && r2.first <= r1.second) || (r1.first <= r2.second && r2.second <= r1.second) ||
           (r2.first <= r1.first && r1.first <= r2.second) || (r2.first <= r1.second && r1.second <= r2.second);
}

void CSMTools::TopicInfoCheckStage::testIntRange(const std::pair<int,int>& range, char op, const ESM::Variant& variant,
        const std::string& name, const CSMWorld::UniversalId& id, CSMDoc::Messages& messages)
{
    if (variant.getType() != ESM::VT_Int && variant.getType() != ESM::VT_Short && variant.getType() != ESM::VT_Long)
    {
        messages.add(id, "Expected an integer type for Info Condition: " + name, "", CSMDoc::Message::Severity_Error);
    }
    else if (!rangeIntersects(range, getRange(op, variant.getInteger(), true, id, messages)) &&
             !rangeIntersects(range, getRange(op, variant.getInteger(), false, id, messages)))
    {
        messages.add(id, "Unattainable Info Condition: " + name, "", CSMDoc::Message::Severity_Error);
    }
}

std::string CSMTools::TopicInfoCheckStage::getFunctionName(int code)
{
    switch (code)
    {
        case 0: return "Rank Low";
        case 1: return "Rank High";
        case 2: return "Rank Requirement";
        case 3: return "Reputation";
        case 4: return "Health Percent";
        case 5: return "PC Reputation";
        case 6: return "PC Level";
        case 7: return "PC Health Percent";
        case 8: return "PC Magicka";
        case 9: return "PC Fatigue";
        case 10: return "PC Strength";
        case 11: return "PC Block";
        case 12: return "PC Armorer";
        case 13: return "PC Medium Armor";
        case 14: return "PC Heavy Armor";
        case 15: return "PC Blunt Weapon";
        case 16: return "PC Long Blade";
        case 17: return "PC Axe";
        case 18: return "PC Spear";
        case 19: return "PC Athletics";
        case 20: return "PC Enchant";
        case 21: return "PC Destruction";
        case 22: return "PC Alteration";
        case 23: return "PC Illusion";
        case 24: return "PC Conjuration";
        case 25: return "PC Mysticism";
        case 26: return "PC Restoration";
        case 27: return "PC Alchemy";
        case 28: return "PC Unarmored";
        case 29: return "PC Security";
        case 30: return "PC Sneak";
        case 31: return "PC Acrobatics";
        case 32: return "PC Light Armor";
        case 33: return "PC Short Blade";
        case 34: return "PC Marksman";
        case 35: return "PC Merchantile";
        case 36: return "PC Speechcraft";
        case 37: return "PC Hand To Hand";
        case 38: return "PC Sex";
        case 39: return "PC Expelled";
        case 40: return "PC Common Disease";
        case 41: return "PC Blight Disease";
        case 42: return "PC Clothing Modifier";
        case 43: return "PC Crime Level";
        case 44: return "Same Sex";
        case 45: return "Same Race";
        case 46: return "Same Faction";
        case 47: return "Faction Rank Difference";
        case 48: return "Detected";
        case 49: return "Alarmed";
        case 50: return "Choice";
        case 51: return "PC Intelligence";
        case 52: return "PC Willpower";
        case 53: return "PC Agility";
        case 54: return "PC Speed";
        case 55: return "PC Endurance";
        case 56: return "PC Personality";
        case 57: return "PC Luck";
        case 58: return "PC Corpus";
        case 59: return "Weather";
        case 60: return "PC Vampire";
        case 61: return "Level";
        case 62: return "Attacked";
        case 63: return "Talked To PC";
        case 64: return "PC Health";
        case 65: return "Creature Target";
        case 66: return "Friend Hit";
        case 67: return "Fight";
        case 68: return "Hello";
        case 69: return "Alarm";
        case 70: return "Flee";
        case 71: return "Should Attack";
        case 72: return "Werewolf";
        case 73: return "PC Werewolf Kills";
        default: return "Unknown (Error)";
    }
}
