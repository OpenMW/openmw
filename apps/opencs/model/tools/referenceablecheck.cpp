#include "referenceablecheck.hpp"

#include <components/misc/stringops.hpp>

#include "../prefs/state.hpp"

#include "../world/record.hpp"
#include "../world/universalid.hpp"

CSMTools::ReferenceableCheckStage::ReferenceableCheckStage(
    const CSMWorld::RefIdData& referenceable, const CSMWorld::IdCollection<ESM::Race >& races,
    const CSMWorld::IdCollection<ESM::Class>& classes,
    const CSMWorld::IdCollection<ESM::Faction>& faction,
    const CSMWorld::IdCollection<ESM::Script>& scripts)
    :
    mReferencables(referenceable),
    mRaces(races),
    mClasses(classes),
    mFactions(faction),
    mScripts(scripts),
    mPlayerPresent(false)
{
    mIgnoreBaseRecords = false;
}

void CSMTools::ReferenceableCheckStage::perform (int stage, CSMDoc::Messages& messages)
{
    //Checks for books, than, when stage is above mBooksSize goes to other checks, with (stage - PrevSum) as stage.
    const int bookSize(mReferencables.getBooks().getSize());

    if (stage < bookSize)
    {
        bookCheck(stage, mReferencables.getBooks(), messages);
        return;
    }

    stage -= bookSize;

    const int activatorSize(mReferencables.getActivators().getSize());

    if (stage < activatorSize)
    {
        activatorCheck(stage, mReferencables.getActivators(), messages);
        return;
    }

    stage -= activatorSize;

    const int potionSize(mReferencables.getPotions().getSize());

    if (stage < potionSize)
    {
        potionCheck(stage, mReferencables.getPotions(), messages);
        return;
    }

    stage -= potionSize;

    const int apparatusSize(mReferencables.getApparati().getSize());

    if (stage < apparatusSize)
    {
        apparatusCheck(stage, mReferencables.getApparati(), messages);
        return;
    }

    stage -= apparatusSize;

    const int armorSize(mReferencables.getArmors().getSize());

    if (stage < armorSize)
    {
        armorCheck(stage, mReferencables.getArmors(), messages);
        return;
    }

    stage -= armorSize;

    const int clothingSize(mReferencables.getClothing().getSize());

    if (stage < clothingSize)
    {
        clothingCheck(stage, mReferencables.getClothing(), messages);
        return;
    }

    stage -= clothingSize;

    const int containerSize(mReferencables.getContainers().getSize());

    if (stage < containerSize)
    {
        containerCheck(stage, mReferencables.getContainers(), messages);
        return;
    }

    stage -= containerSize;

    const int doorSize(mReferencables.getDoors().getSize());

    if (stage < doorSize)
    {
        doorCheck(stage, mReferencables.getDoors(), messages);
        return;
    }

    stage -= doorSize;

    const int ingredientSize(mReferencables.getIngredients().getSize());

    if (stage < ingredientSize)
    {
        ingredientCheck(stage, mReferencables.getIngredients(), messages);
        return;
    }

    stage -= ingredientSize;

    const int creatureLevListSize(mReferencables.getCreatureLevelledLists().getSize());

    if (stage < creatureLevListSize)
    {
        creaturesLevListCheck(stage, mReferencables.getCreatureLevelledLists(), messages);
        return;
    }

    stage -= creatureLevListSize;

    const int itemLevelledListSize(mReferencables.getItemLevelledList().getSize());

    if (stage < itemLevelledListSize)
    {
        itemLevelledListCheck(stage, mReferencables.getItemLevelledList(), messages);
        return;
    }

    stage -= itemLevelledListSize;

    const int lightSize(mReferencables.getLights().getSize());

    if (stage < lightSize)
    {
        lightCheck(stage, mReferencables.getLights(), messages);
        return;
    }

    stage -= lightSize;

    const int lockpickSize(mReferencables.getLocpicks().getSize());

    if (stage < lockpickSize)
    {
        lockpickCheck(stage, mReferencables.getLocpicks(), messages);
        return;
    }

    stage -= lockpickSize;

    const int miscSize(mReferencables.getMiscellaneous().getSize());

    if (stage < miscSize)
    {
        miscCheck(stage, mReferencables.getMiscellaneous(), messages);
        return;
    }

    stage -= miscSize;

    const int npcSize(mReferencables.getNPCs().getSize());

    if (stage < npcSize)
    {
        npcCheck(stage, mReferencables.getNPCs(), messages);
        return;
    }

    stage -= npcSize;

    const int weaponSize(mReferencables.getWeapons().getSize());

    if (stage < weaponSize)
    {
        weaponCheck(stage, mReferencables.getWeapons(), messages);
        return;
    }

    stage -= weaponSize;

    const int probeSize(mReferencables.getProbes().getSize());

    if (stage < probeSize)
    {
        probeCheck(stage, mReferencables.getProbes(), messages);
        return;
    }

    stage -= probeSize;

    const int repairSize(mReferencables.getRepairs().getSize());

    if (stage < repairSize)
    {
        repairCheck(stage, mReferencables.getRepairs(), messages);
        return;
    }

    stage -= repairSize;

    const int staticSize(mReferencables.getStatics().getSize());

    if (stage < staticSize)
    {
        staticCheck(stage, mReferencables.getStatics(), messages);
        return;
    }

    stage -= staticSize;

    const int creatureSize(mReferencables.getCreatures().getSize());

    if (stage < creatureSize)
    {
        creatureCheck(stage, mReferencables.getCreatures(), messages);
        return;
    }
// if we come that far, we are about to perform our last, final check.
    finalCheck(messages);
    return;
}

int CSMTools::ReferenceableCheckStage::setup()
{
    mPlayerPresent = false;
    mIgnoreBaseRecords = CSMPrefs::get()["Reports"]["ignore-base-records"].isTrue();

    return mReferencables.getSize() + 1;
}

void CSMTools::ReferenceableCheckStage::bookCheck(
    int stage,
    const CSMWorld::RefIdDataContainer< ESM::Book >& records,
    CSMDoc::Messages& messages)
{
    const CSMWorld::RecordBase& baseRecord = records.getRecord(stage);

    // Skip "Base" records (setting!) and "Deleted" records
    if ((mIgnoreBaseRecords && baseRecord.mState == CSMWorld::RecordBase::State_BaseOnly) || baseRecord.isDeleted())
        return;

    const ESM::Book& book = (dynamic_cast<const CSMWorld::Record<ESM::Book>& >(baseRecord)).get();
    CSMWorld::UniversalId id(CSMWorld::UniversalId::Type_Book, book.mId);

    inventoryItemCheck<ESM::Book>(book, messages, id.toString(), true);

    // Check that mentioned scripts exist
    scriptCheck<ESM::Book>(book, messages, id.toString());
}

void CSMTools::ReferenceableCheckStage::activatorCheck(
    int stage,
    const CSMWorld::RefIdDataContainer< ESM::Activator >& records,
    CSMDoc::Messages& messages)
{
    const CSMWorld::RecordBase& baseRecord = records.getRecord(stage);

    // Skip "Base" records (setting!) and "Deleted" records
    if ((mIgnoreBaseRecords && baseRecord.mState == CSMWorld::RecordBase::State_BaseOnly) || baseRecord.isDeleted())
        return;

    const ESM::Activator& activator = (dynamic_cast<const CSMWorld::Record<ESM::Activator>& >(baseRecord)).get();
    CSMWorld::UniversalId id(CSMWorld::UniversalId::Type_Activator, activator.mId);

    //Checking for model, IIRC all activators should have a model
    if (activator.mModel.empty())
        messages.push_back (std::make_pair (id, activator.mId + " has no model"));

    // Check that mentioned scripts exist
    scriptCheck<ESM::Activator>(activator, messages, id.toString());
}

void CSMTools::ReferenceableCheckStage::potionCheck(
    int stage,
    const CSMWorld::RefIdDataContainer< ESM::Potion >& records,
    CSMDoc::Messages& messages)
{
    const CSMWorld::RecordBase& baseRecord = records.getRecord(stage);

    // Skip "Base" records (setting!) and "Deleted" records
    if ((mIgnoreBaseRecords && baseRecord.mState == CSMWorld::RecordBase::State_BaseOnly) || baseRecord.isDeleted())
        return;

    const ESM::Potion& potion = (dynamic_cast<const CSMWorld::Record<ESM::Potion>& >(baseRecord)).get();
    CSMWorld::UniversalId id(CSMWorld::UniversalId::Type_Potion, potion.mId);

    inventoryItemCheck<ESM::Potion>(potion, messages, id.toString());
    //IIRC potion can have empty effects list just fine.

    // Check that mentioned scripts exist
    scriptCheck<ESM::Potion>(potion, messages, id.toString());
}


void CSMTools::ReferenceableCheckStage::apparatusCheck(
    int stage,
    const CSMWorld::RefIdDataContainer< ESM::Apparatus >& records,
    CSMDoc::Messages& messages)
{
    const CSMWorld::RecordBase& baseRecord = records.getRecord(stage);

    // Skip "Base" records (setting!) and "Deleted" records
    if ((mIgnoreBaseRecords && baseRecord.mState == CSMWorld::RecordBase::State_BaseOnly) || baseRecord.isDeleted())
        return;

    const ESM::Apparatus& apparatus = (dynamic_cast<const CSMWorld::Record<ESM::Apparatus>& >(baseRecord)).get();
    CSMWorld::UniversalId id(CSMWorld::UniversalId::Type_Apparatus, apparatus.mId);

    inventoryItemCheck<ESM::Apparatus>(apparatus, messages, id.toString());

    toolCheck<ESM::Apparatus>(apparatus, messages, id.toString());

    // Check that mentioned scripts exist
    scriptCheck<ESM::Apparatus>(apparatus, messages, id.toString());
}

void CSMTools::ReferenceableCheckStage::armorCheck(
    int stage,
    const CSMWorld::RefIdDataContainer< ESM::Armor >& records,
    CSMDoc::Messages& messages)
{
    const CSMWorld::RecordBase& baseRecord = records.getRecord(stage);

    // Skip "Base" records (setting!) and "Deleted" records
    if ((mIgnoreBaseRecords && baseRecord.mState == CSMWorld::RecordBase::State_BaseOnly) || baseRecord.isDeleted())
        return;

    const ESM::Armor& armor = (dynamic_cast<const CSMWorld::Record<ESM::Armor>& >(baseRecord)).get();
    CSMWorld::UniversalId id(CSMWorld::UniversalId::Type_Armor, armor.mId);

    inventoryItemCheck<ESM::Armor>(armor, messages, id.toString(), true);

    //checking for armor class, armor should have poistive armor class, but 0 is considered legal
    if (armor.mData.mArmor < 0)
        messages.push_back (std::make_pair (id, armor.mId + " has negative armor class"));

    //checking for health. Only positive numbers are allowed, or 0 is illegal
    if (armor.mData.mHealth <= 0)
        messages.push_back (std::make_pair (id, armor.mId + " has non positive health"));

    // Check that mentioned scripts exist
    scriptCheck<ESM::Armor>(armor, messages, id.toString());
}

void CSMTools::ReferenceableCheckStage::clothingCheck(
    int stage,
    const CSMWorld::RefIdDataContainer< ESM::Clothing >& records,
    CSMDoc::Messages& messages)
{
    const CSMWorld::RecordBase& baseRecord = records.getRecord(stage);

    // Skip "Base" records (setting!) and "Deleted" records
    if ((mIgnoreBaseRecords && baseRecord.mState == CSMWorld::RecordBase::State_BaseOnly) || baseRecord.isDeleted())
        return;

    const ESM::Clothing& clothing = (dynamic_cast<const CSMWorld::Record<ESM::Clothing>& >(baseRecord)).get();
    CSMWorld::UniversalId id(CSMWorld::UniversalId::Type_Clothing, clothing.mId);
    inventoryItemCheck<ESM::Clothing>(clothing, messages, id.toString(), true);

    // Check that mentioned scripts exist
    scriptCheck<ESM::Clothing>(clothing, messages, id.toString());
}

void CSMTools::ReferenceableCheckStage::containerCheck(
    int stage,
    const CSMWorld::RefIdDataContainer< ESM::Container >& records,
    CSMDoc::Messages& messages)
{
    const CSMWorld::RecordBase& baseRecord = records.getRecord(stage);

    // Skip "Base" records (setting!) and "Deleted" records
    if ((mIgnoreBaseRecords && baseRecord.mState == CSMWorld::RecordBase::State_BaseOnly) || baseRecord.isDeleted())
        return;

    const ESM::Container& container = (dynamic_cast<const CSMWorld::Record<ESM::Container>& >(baseRecord)).get();
    CSMWorld::UniversalId id(CSMWorld::UniversalId::Type_Container, container.mId);

    //Checking for model, IIRC all containers should have a model
    if (container.mModel.empty())
        messages.push_back (std::make_pair (id, container.mId + " has no model"));

    //Checking for capacity (weight)
    if (container.mWeight < 0) //0 is allowed
        messages.push_back (std::make_pair (id,
            container.mId + " has negative weight (capacity)"));

    //checking for name
    if (container.mName.empty())
        messages.push_back (std::make_pair (id, container.mId + " has an empty name"));
    
    //checking contained items
    inventoryListCheck(container.mInventory.mList, messages, id.toString());

    // Check that mentioned scripts exist
    scriptCheck<ESM::Container>(container, messages, id.toString());
}

void CSMTools::ReferenceableCheckStage::creatureCheck (
    int stage, const CSMWorld::RefIdDataContainer< ESM::Creature >& records,
    CSMDoc::Messages& messages)
{
    const CSMWorld::RecordBase& baseRecord = records.getRecord(stage);

    // Skip "Base" records (setting!) and "Deleted" records
    if ((mIgnoreBaseRecords && baseRecord.mState == CSMWorld::RecordBase::State_BaseOnly) || baseRecord.isDeleted())
        return;

    const ESM::Creature& creature = (dynamic_cast<const CSMWorld::Record<ESM::Creature>&>(baseRecord)).get();
    CSMWorld::UniversalId id(CSMWorld::UniversalId::Type_Creature, creature.mId);

    if (creature.mModel.empty())
        messages.push_back (std::make_pair (id, creature.mId + " has no model"));

    if (creature.mName.empty())
        messages.push_back (std::make_pair (id, creature.mId + " has an empty name"));

    //stats checks
    if (creature.mData.mLevel < 1)
        messages.push_back (std::make_pair (id, creature.mId + " has non-positive level"));

    if (creature.mData.mStrength < 0)
        messages.push_back (std::make_pair (id, creature.mId + " has negative strength"));

    if (creature.mData.mIntelligence < 0)
        messages.push_back (std::make_pair (id, creature.mId + " has negative intelligence"));

    if (creature.mData.mWillpower < 0)
        messages.push_back (std::make_pair (id, creature.mId + " has negative willpower"));

    if (creature.mData.mAgility < 0)
        messages.push_back (std::make_pair (id, creature.mId + " has negative agility"));

    if (creature.mData.mSpeed < 0)
        messages.push_back (std::make_pair (id, creature.mId + " has negative speed"));

    if (creature.mData.mEndurance < 0)
        messages.push_back (std::make_pair (id, creature.mId + " has negative endurance"));

    if (creature.mData.mPersonality < 0)
        messages.push_back (std::make_pair (id, creature.mId + " has negative personality"));

    if (creature.mData.mLuck < 0)
        messages.push_back (std::make_pair (id, creature.mId + " has negative luck"));

    if (creature.mData.mHealth < 0)
        messages.push_back (std::make_pair (id, creature.mId + " has negative health"));

    if (creature.mData.mSoul < 0)
        messages.push_back (std::make_pair (id, creature.mId + " has negative soul value"));

    for (int i = 0; i < 6; ++i)
    {
        if (creature.mData.mAttack[i] < 0)
        {
            messages.push_back (std::make_pair (id,
                creature.mId + " has negative attack strength"));
            break;
        }
    }

    //TODO, find meaning of other values
    if (creature.mData.mGold < 0) //It seems that this is for gold in merchant creatures
        messages.push_back (std::make_pair (id, creature.mId + " has negative gold "));

    if (creature.mScale == 0)
        messages.push_back (std::make_pair (id, creature.mId + " has zero scale value"));

    // Check inventory
    inventoryListCheck(creature.mInventory.mList, messages, id.toString());
 
    // Check that mentioned scripts exist
    scriptCheck<ESM::Creature>(creature, messages, id.toString());
}

void CSMTools::ReferenceableCheckStage::doorCheck(
    int stage, const CSMWorld::RefIdDataContainer< ESM::Door >& records,
    CSMDoc::Messages& messages)
{
    const CSMWorld::RecordBase& baseRecord = records.getRecord(stage);

    // Skip "Base" records (setting!) and "Deleted" records
    if ((mIgnoreBaseRecords && baseRecord.mState == CSMWorld::RecordBase::State_BaseOnly) || baseRecord.isDeleted())
        return;

    const ESM::Door& door = (dynamic_cast<const CSMWorld::Record<ESM::Door>&>(baseRecord)).get();
    CSMWorld::UniversalId id(CSMWorld::UniversalId::Type_Door, door.mId);

    //usual, name or model
    if (door.mName.empty())
        messages.push_back (std::make_pair (id, door.mId + " has an empty name"));

    if (door.mModel.empty())
        messages.push_back (std::make_pair (id, door.mId + " has no model"));

    // Check that mentioned scripts exist
    scriptCheck<ESM::Door>(door, messages, id.toString());
}

void CSMTools::ReferenceableCheckStage::ingredientCheck(
    int stage,
    const CSMWorld::RefIdDataContainer< ESM::Ingredient >& records,
    CSMDoc::Messages& messages)
{
    const CSMWorld::RecordBase& baseRecord = records.getRecord(stage);

    // Skip "Base" records (setting!) and "Deleted" records
    if ((mIgnoreBaseRecords && baseRecord.mState == CSMWorld::RecordBase::State_BaseOnly) || baseRecord.isDeleted())
        return;

    const ESM::Ingredient& ingredient = (dynamic_cast<const CSMWorld::Record<ESM::Ingredient>& >(baseRecord)).get();
    CSMWorld::UniversalId id(CSMWorld::UniversalId::Type_Ingredient, ingredient.mId);

    inventoryItemCheck<ESM::Ingredient>(ingredient, messages, id.toString());

    // Check that mentioned scripts exist
    scriptCheck<ESM::Ingredient>(ingredient, messages, id.toString());
}

void CSMTools::ReferenceableCheckStage::creaturesLevListCheck(
    int stage,
    const CSMWorld::RefIdDataContainer< ESM::CreatureLevList >& records,
    CSMDoc::Messages& messages)
{
    const CSMWorld::RecordBase& baseRecord = records.getRecord(stage);

    // Skip "Base" records (setting!) and "Deleted" records
    if ((mIgnoreBaseRecords && baseRecord.mState == CSMWorld::RecordBase::State_BaseOnly) || baseRecord.isDeleted())
        return;

    const ESM::CreatureLevList& CreatureLevList = (dynamic_cast<const CSMWorld::Record<ESM::CreatureLevList>& >(baseRecord)).get();
    CSMWorld::UniversalId id(CSMWorld::UniversalId::Type_CreatureLevelledList, CreatureLevList.mId); //CreatureLevList but Type_CreatureLevelledList :/

    listCheck<ESM::CreatureLevList>(CreatureLevList, messages, id.toString());
}

void CSMTools::ReferenceableCheckStage::itemLevelledListCheck(
    int stage,
    const CSMWorld::RefIdDataContainer< ESM::ItemLevList >& records,
    CSMDoc::Messages& messages)
{
    const CSMWorld::RecordBase& baseRecord = records.getRecord(stage);

    // Skip "Base" records (setting!) and "Deleted" records
    if ((mIgnoreBaseRecords && baseRecord.mState == CSMWorld::RecordBase::State_BaseOnly) || baseRecord.isDeleted())
        return;

    const ESM::ItemLevList& ItemLevList = (dynamic_cast<const CSMWorld::Record<ESM::ItemLevList>& >(baseRecord)).get();
    CSMWorld::UniversalId id(CSMWorld::UniversalId::Type_ItemLevelledList, ItemLevList.mId);

    listCheck<ESM::ItemLevList>(ItemLevList, messages, id.toString());
}

void CSMTools::ReferenceableCheckStage::lightCheck(
    int stage, const CSMWorld::RefIdDataContainer< ESM::Light >& records,
    CSMDoc::Messages& messages)
{
    const CSMWorld::RecordBase& baseRecord = records.getRecord(stage);

    // Skip "Base" records (setting!) and "Deleted" records
    if ((mIgnoreBaseRecords && baseRecord.mState == CSMWorld::RecordBase::State_BaseOnly) || baseRecord.isDeleted())
        return;

    const ESM::Light& light = (dynamic_cast<const CSMWorld::Record<ESM::Light>& >(baseRecord)).get();
    CSMWorld::UniversalId id(CSMWorld::UniversalId::Type_Light, light.mId);

    if (light.mData.mRadius < 0)
        messages.push_back (std::make_pair (id, light.mId + " has negative light radius"));

    if (light.mData.mFlags & ESM::Light::Carry)
        inventoryItemCheck<ESM::Light>(light, messages, id.toString());

    // Check that mentioned scripts exist
    scriptCheck<ESM::Light>(light, messages, id.toString());
}

void CSMTools::ReferenceableCheckStage::lockpickCheck(
    int stage,
    const CSMWorld::RefIdDataContainer< ESM::Lockpick >& records,
    CSMDoc::Messages& messages)
{
    const CSMWorld::RecordBase& baseRecord = records.getRecord(stage);

    // Skip "Base" records (setting!) and "Deleted" records
    if ((mIgnoreBaseRecords && baseRecord.mState == CSMWorld::RecordBase::State_BaseOnly) || baseRecord.isDeleted())
        return;

    const ESM::Lockpick& lockpick = (dynamic_cast<const CSMWorld::Record<ESM::Lockpick>& >(baseRecord)).get();
    CSMWorld::UniversalId id(CSMWorld::UniversalId::Type_Lockpick, lockpick.mId);

    inventoryItemCheck<ESM::Lockpick>(lockpick, messages, id.toString());

    toolCheck<ESM::Lockpick>(lockpick, messages, id.toString(), true);

    // Check that mentioned scripts exist
    scriptCheck<ESM::Lockpick>(lockpick, messages, id.toString());
}

void CSMTools::ReferenceableCheckStage::miscCheck(
    int stage,
    const CSMWorld::RefIdDataContainer< ESM::Miscellaneous >& records,
    CSMDoc::Messages& messages)
{
    const CSMWorld::RecordBase& baseRecord = records.getRecord(stage);

    // Skip "Base" records (setting!) and "Deleted" records
    if ((mIgnoreBaseRecords && baseRecord.mState == CSMWorld::RecordBase::State_BaseOnly) || baseRecord.isDeleted())
        return;

    const ESM::Miscellaneous& miscellaneous = (dynamic_cast<const CSMWorld::Record<ESM::Miscellaneous>& >(baseRecord)).get();
    CSMWorld::UniversalId id(CSMWorld::UniversalId::Type_Miscellaneous, miscellaneous.mId);

    inventoryItemCheck<ESM::Miscellaneous>(miscellaneous, messages, id.toString());

    // Check that mentioned scripts exist
    scriptCheck<ESM::Miscellaneous>(miscellaneous, messages, id.toString());
}

void CSMTools::ReferenceableCheckStage::npcCheck (
    int stage, const CSMWorld::RefIdDataContainer< ESM::NPC >& records,
    CSMDoc::Messages& messages)
{
    const CSMWorld::RecordBase& baseRecord = records.getRecord(stage);

    if (baseRecord.isDeleted())
        return;

    const ESM::NPC& npc = (dynamic_cast<const CSMWorld::Record<ESM::NPC>& >(baseRecord)).get();
    CSMWorld::UniversalId id (CSMWorld::UniversalId::Type_Npc, npc.mId);

    //Detect if player is present
    if (Misc::StringUtils::ciEqual(npc.mId, "player")) //Happy now, scrawl?
        mPlayerPresent = true;

    // Skip "Base" records (setting!)
    if (mIgnoreBaseRecords && baseRecord.mState == CSMWorld::RecordBase::State_BaseOnly)
        return;

    short level(npc.mNpdt.mLevel);
    char disposition(npc.mNpdt.mDisposition);
    char reputation(npc.mNpdt.mReputation);
    char rank(npc.mNpdt.mRank);
    //Don't know what unknown is for
    int gold(npc.mNpdt.mGold);

    if (npc.mNpdtType == ESM::NPC::NPC_WITH_AUTOCALCULATED_STATS) //12 = autocalculated
    {
        if ((npc.mFlags & ESM::NPC::Autocalc) == 0) //0x0010 = autocalculated flag
        {
            messages.push_back (std::make_pair (id, npc.mId + " mNpdtType or flags mismatch!")); //should not happen?
            return;
        }

        level = npc.mNpdt.mLevel;
        disposition = npc.mNpdt.mDisposition;
        reputation = npc.mNpdt.mReputation;
        rank = npc.mNpdt.mRank;
        gold = npc.mNpdt.mGold;
    }
    else
    {
        if (npc.mNpdt.mAgility == 0)
            messages.push_back (std::make_pair (id, npc.mId + " agility has zero value"));

        if (npc.mNpdt.mEndurance == 0)
            messages.push_back (std::make_pair (id, npc.mId + " endurance has zero value"));

        if (npc.mNpdt.mIntelligence == 0)
            messages.push_back (std::make_pair (id, npc.mId + " intelligence has zero value"));

        if (npc.mNpdt.mLuck == 0)
            messages.push_back (std::make_pair (id, npc.mId + " luck has zero value"));

        if (npc.mNpdt.mPersonality == 0)
            messages.push_back (std::make_pair (id, npc.mId + " personality has zero value"));

        if (npc.mNpdt.mStrength == 0)
            messages.push_back (std::make_pair (id, npc.mId + " strength has zero value"));

        if (npc.mNpdt.mSpeed == 0)
            messages.push_back (std::make_pair (id, npc.mId + " speed has zero value"));

        if (npc.mNpdt.mWillpower == 0)
            messages.push_back (std::make_pair (id, npc.mId + " willpower has zero value"));
    }

    if (level < 1)
        messages.push_back (std::make_pair (id, npc.mId + " level is non positive"));

    if (gold < 0)
        messages.push_back (std::make_pair (id, npc.mId + " gold has negative value"));

    if (npc.mName.empty())
        messages.push_back (std::make_pair (id, npc.mId + " has any empty name"));

    if (npc.mClass.empty())
        messages.push_back (std::make_pair (id, npc.mId + " has an empty class"));
    else if (mClasses.searchId (npc.mClass) == -1)
        messages.push_back (std::make_pair (id, npc.mId + " has invalid class"));

    if (npc.mRace.empty())
        messages.push_back (std::make_pair (id, npc.mId + " has an empty race"));
    else if (mRaces.searchId (npc.mRace) == -1)
        messages.push_back (std::make_pair (id, npc.mId + " has invalid race"));

    if (disposition < 0)
        messages.push_back (std::make_pair (id, npc.mId + " has negative disposition"));

    if (reputation < 0) //It seems that no character in Morrowind.esm have negative reputation. I'm assuming that negative reputation is invalid
    {
        messages.push_back (std::make_pair (id, npc.mId + " has negative reputation"));
    }

    if (!npc.mFaction.empty())
    {
        if (rank < 0)
            messages.push_back (std::make_pair (id, npc.mId + " has negative rank"));

        if (mFactions.searchId(npc.mFaction) == -1)
            messages.push_back (std::make_pair (id, npc.mId + " has invalid faction"));
    }

    if (npc.mHead.empty())
        messages.push_back (std::make_pair (id, npc.mId + " has no head"));

    if (npc.mHair.empty())
        messages.push_back (std::make_pair (id, npc.mId + " has no hair"));

    //TODO: reputation, Disposition, rank, everything else

    // Check inventory
    inventoryListCheck(npc.mInventory.mList, messages, id.toString());
 
    // Check that mentioned scripts exist
    scriptCheck<ESM::NPC>(npc, messages, id.toString());
}

void CSMTools::ReferenceableCheckStage::weaponCheck(
    int stage, const CSMWorld::RefIdDataContainer< ESM::Weapon >& records,
    CSMDoc::Messages& messages)
{
    const CSMWorld::RecordBase& baseRecord = records.getRecord (stage);

    // Skip "Base" records (setting!) and "Deleted" records
    if ((mIgnoreBaseRecords && baseRecord.mState == CSMWorld::RecordBase::State_BaseOnly) || baseRecord.isDeleted())
        return;

    const ESM::Weapon& weapon = (dynamic_cast<const CSMWorld::Record<ESM::Weapon>& >(baseRecord)).get();
    CSMWorld::UniversalId id (CSMWorld::UniversalId::Type_Weapon, weapon.mId);

    //TODO, It seems that this stuff for spellcasting is obligatory and In fact We should check if records are present
    if
    (   //THOSE ARE HARDCODED!
        !(weapon.mId == "VFX_Hands" ||
          weapon.mId == "VFX_Absorb" ||
          weapon.mId == "VFX_Reflect" ||
          weapon.mId == "VFX_DefaultBolt" ||
          //TODO I don't know how to get full list of effects :/
          //DANGER!, ACHTUNG! FIXME! The following is the list of the magical bolts, valid for Morrowind.esm. However those are not hardcoded.
          weapon.mId == "magic_bolt" ||
          weapon.mId == "shock_bolt" ||
          weapon.mId == "shield_bolt" ||
          weapon.mId == "VFX_DestructBolt" ||
          weapon.mId == "VFX_PoisonBolt" ||
          weapon.mId == "VFX_RestoreBolt" ||
          weapon.mId == "VFX_AlterationBolt" ||
          weapon.mId == "VFX_ConjureBolt" ||
          weapon.mId == "VFX_FrostBolt" ||
          weapon.mId == "VFX_MysticismBolt" ||
          weapon.mId == "VFX_IllusionBolt" ||
          weapon.mId == "VFX_Multiple2" ||
          weapon.mId == "VFX_Multiple3" ||
          weapon.mId == "VFX_Multiple4" ||
          weapon.mId == "VFX_Multiple5" ||
          weapon.mId == "VFX_Multiple6" ||
          weapon.mId == "VFX_Multiple7" ||
          weapon.mId == "VFX_Multiple8" ||
          weapon.mId == "VFX_Multiple9"))
    {
        inventoryItemCheck<ESM::Weapon>(weapon, messages, id.toString(), true);

        if (!(weapon.mData.mType == ESM::Weapon::MarksmanBow ||
                weapon.mData.mType == ESM::Weapon::MarksmanCrossbow ||
                weapon.mData.mType == ESM::Weapon::MarksmanThrown ||
                weapon.mData.mType == ESM::Weapon::Arrow ||
                weapon.mData.mType == ESM::Weapon::Bolt))
        {
            if (weapon.mData.mSlash[0] > weapon.mData.mSlash[1])
                messages.push_back (std::make_pair (id,
                    weapon.mId + " has minimum slash damage higher than maximum"));

            if (weapon.mData.mThrust[0] > weapon.mData.mThrust[1])
                messages.push_back (std::make_pair (id,
                    weapon.mId + " has minimum thrust damage higher than maximum"));
        }

        if (weapon.mData.mChop[0] > weapon.mData.mChop[1])
            messages.push_back (std::make_pair (id,
                weapon.mId + " has minimum chop damage higher than maximum"));

        if (!(weapon.mData.mType == ESM::Weapon::Arrow ||
                weapon.mData.mType == ESM::Weapon::Bolt ||
                weapon.mData.mType == ESM::Weapon::MarksmanThrown))
        {
            //checking of health
            if (weapon.mData.mHealth <= 0)
                messages.push_back (std::make_pair (id, weapon.mId + " has non-positive health"));

            if (weapon.mData.mReach < 0)
                messages.push_back (std::make_pair (id, weapon.mId + " has negative reach"));
        }
    }

    // Check that mentioned scripts exist
    scriptCheck<ESM::Weapon>(weapon, messages, id.toString());
}

void CSMTools::ReferenceableCheckStage::probeCheck(
    int stage,
    const CSMWorld::RefIdDataContainer< ESM::Probe >& records,
    CSMDoc::Messages& messages)
{
    const CSMWorld::RecordBase& baseRecord = records.getRecord(stage);

    // Skip "Base" records (setting!) and "Deleted" records
    if ((mIgnoreBaseRecords && baseRecord.mState == CSMWorld::RecordBase::State_BaseOnly) || baseRecord.isDeleted())
        return;

    const ESM::Probe& probe = (dynamic_cast<const CSMWorld::Record<ESM::Probe>& >(baseRecord)).get();
    CSMWorld::UniversalId id(CSMWorld::UniversalId::Type_Probe, probe.mId);

    inventoryItemCheck<ESM::Probe>(probe, messages, id.toString());
    toolCheck<ESM::Probe>(probe, messages, id.toString(), true);

    // Check that mentioned scripts exist
    scriptCheck<ESM::Probe>(probe, messages, id.toString());
}

void CSMTools::ReferenceableCheckStage::repairCheck (
    int stage, const CSMWorld::RefIdDataContainer< ESM::Repair >& records,
    CSMDoc::Messages& messages)
{
    const CSMWorld::RecordBase& baseRecord = records.getRecord (stage);

    // Skip "Base" records (setting!) and "Deleted" records
    if ((mIgnoreBaseRecords && baseRecord.mState == CSMWorld::RecordBase::State_BaseOnly) || baseRecord.isDeleted())
        return;

    const ESM::Repair& repair = (dynamic_cast<const CSMWorld::Record<ESM::Repair>& >(baseRecord)).get();
    CSMWorld::UniversalId id (CSMWorld::UniversalId::Type_Repair, repair.mId);

    inventoryItemCheck<ESM::Repair> (repair, messages, id.toString());
    toolCheck<ESM::Repair> (repair, messages, id.toString(), true);

    // Check that mentioned scripts exist
    scriptCheck<ESM::Repair>(repair, messages, id.toString());
}

void CSMTools::ReferenceableCheckStage::staticCheck (
    int stage, const CSMWorld::RefIdDataContainer< ESM::Static >& records,
    CSMDoc::Messages& messages)
{
    const CSMWorld::RecordBase& baseRecord = records.getRecord (stage);

    // Skip "Base" records (setting!) and "Deleted" records
    if ((mIgnoreBaseRecords && baseRecord.mState == CSMWorld::RecordBase::State_BaseOnly) || baseRecord.isDeleted())
        return;

    const ESM::Static& staticElement = (dynamic_cast<const CSMWorld::Record<ESM::Static>& >(baseRecord)).get();
    CSMWorld::UniversalId id (CSMWorld::UniversalId::Type_Static, staticElement.mId);

    if (staticElement.mModel.empty())
        messages.push_back (std::make_pair (id, staticElement.mId + " has no model"));
}

//final check

void CSMTools::ReferenceableCheckStage::finalCheck (CSMDoc::Messages& messages)
{
    if (!mPlayerPresent)
        messages.push_back (std::make_pair (CSMWorld::UniversalId::Type_Referenceables,
            "There is no player record"));
}

void CSMTools::ReferenceableCheckStage::inventoryListCheck(
    const std::vector<ESM::ContItem>& itemList, 
    CSMDoc::Messages& messages, 
    const std::string& id)
{
    for (size_t i = 0; i < itemList.size(); ++i)
    {
        std::string itemName = itemList[i].mItem.toString();
        CSMWorld::RefIdData::LocalIndex localIndex = mReferencables.searchId(itemName);

        if (localIndex.first == -1)
            messages.push_back (std::make_pair (id,
                id + " contains non-existing item (" + itemName + ")"));
        else
        {
            // Needs to accommodate containers, creatures, and NPCs
            switch (localIndex.second)
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
                messages.push_back (std::make_pair(id,
                    id + " contains item of invalid type (" + itemName + ")"));
            }
        }
    }
}

//Templates begins here

template<typename Item> void CSMTools::ReferenceableCheckStage::inventoryItemCheck (
    const Item& someItem, CSMDoc::Messages& messages, const std::string& someID, bool enchantable)
{
    if (someItem.mName.empty())
        messages.push_back (std::make_pair (someID, someItem.mId + " has an empty name"));

    //Checking for weight
    if (someItem.mData.mWeight < 0)
        messages.push_back (std::make_pair (someID, someItem.mId + " has negative weight"));

    //Checking for value
    if (someItem.mData.mValue < 0)
        messages.push_back (std::make_pair (someID, someItem.mId + " has negative value"));

    //checking for model
    if (someItem.mModel.empty())
        messages.push_back (std::make_pair (someID, someItem.mId + " has no model"));

    //checking for icon
    if (someItem.mIcon.empty())
        messages.push_back (std::make_pair (someID, someItem.mId + " has no icon"));

    if (enchantable && someItem.mData.mEnchant < 0)
        messages.push_back (std::make_pair (someID, someItem.mId + " has negative enchantment"));
}

template<typename Item> void CSMTools::ReferenceableCheckStage::inventoryItemCheck (
    const Item& someItem, CSMDoc::Messages& messages, const std::string& someID)
{
    if (someItem.mName.empty())
        messages.push_back (std::make_pair (someID, someItem.mId + " has an empty name"));

    //Checking for weight
    if (someItem.mData.mWeight < 0)
        messages.push_back (std::make_pair (someID, someItem.mId + " has negative weight"));

    //Checking for value
    if (someItem.mData.mValue < 0)
        messages.push_back (std::make_pair (someID, someItem.mId + " has negative value"));

    //checking for model
    if (someItem.mModel.empty())
        messages.push_back (std::make_pair (someID, someItem.mId + " has no model"));

    //checking for icon
    if (someItem.mIcon.empty())
        messages.push_back (std::make_pair (someID, someItem.mId + " has no icon"));
}

template<typename Tool> void CSMTools::ReferenceableCheckStage::toolCheck (
    const Tool& someTool, CSMDoc::Messages& messages, const std::string& someID, bool canBeBroken)
{
    if (someTool.mData.mQuality <= 0)
        messages.push_back (std::make_pair (someID, someTool.mId + " has non-positive quality"));

    if (canBeBroken && someTool.mData.mUses<=0)
        messages.push_back (std::make_pair (someID,
            someTool.mId + " has non-positive uses count"));
}

template<typename Tool> void CSMTools::ReferenceableCheckStage::toolCheck (
    const Tool& someTool, CSMDoc::Messages& messages, const std::string& someID)
{
    if (someTool.mData.mQuality <= 0)
        messages.push_back (std::make_pair (someID, someTool.mId + " has non-positive quality"));
}

template<typename List> void CSMTools::ReferenceableCheckStage::listCheck (
    const List& someList, CSMDoc::Messages& messages, const std::string& someID)
{
    for (unsigned i = 0; i < someList.mList.size(); ++i)
    {
        if (mReferencables.searchId(someList.mList[i].mId).first == -1)
            messages.push_back (std::make_pair (someID,
                someList.mId + " contains item without referencable"));

        if (someList.mList[i].mLevel < 1)
            messages.push_back (std::make_pair (someID,
                someList.mId + " contains item with non-positive level"));
    }
}

template<typename Tool> void CSMTools::ReferenceableCheckStage::scriptCheck (
    const Tool& someTool, CSMDoc::Messages& messages, const std::string& someID)
{
    if (!someTool.mScript.empty())
    {
        if (mScripts.searchId(someTool.mScript) == -1)
            messages.push_back (std::make_pair (someID, someTool.mId + " refers to an unknown script \""+someTool.mScript+"\""));
    }
}
