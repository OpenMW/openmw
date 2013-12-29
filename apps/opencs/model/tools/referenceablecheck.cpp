#include "referenceablecheck.hpp"

#include <sstream>
#include <map>
#include <cassert>
#include <boost/graph/graph_concepts.hpp>

#include "../world/record.hpp"

#include "../world/universalid.hpp"

CSMTools::ReferenceableCheckStage::ReferenceableCheckStage(const CSMWorld::RefIdData& referenceable, const CSMWorld::IdCollection<ESM::Race >& races, const CSMWorld::IdCollection<ESM::Class>& classes) :
    mReferencables(referenceable),
    mClasses(classes),
    mRaces(races)
{
}

void CSMTools::ReferenceableCheckStage::perform(int stage, std::vector< std::string >& messages)
{
    //Checks for books, than, when stage is above mBooksSize goes to other checks, with (stage - PrevSum) as stage.
    const int BookSize(mReferencables.getBooks().getSize());

    if (stage < BookSize)
    {
        bookCheck(stage, mReferencables.getBooks(), messages);
        return;
    }

    stage -= BookSize;

    const int ActivatorSize(mReferencables.getActivators().getSize());

    if (stage < ActivatorSize)
    {
        activatorCheck(stage, mReferencables.getActivators(), messages);
        return;
    }

    stage -= ActivatorSize;

    const int PotionSize(mReferencables.getActivators().getSize());

    if (stage < PotionSize)
    {
        potionCheck(stage, mReferencables.getPotions(), messages);
        return;
    }

    stage -= PotionSize;

    const int ApparatusSize(mReferencables.getApparati().getSize());

    if (stage < ApparatusSize)
    {
        apparatusCheck(stage, mReferencables.getApparati(), messages);
        return;
    }

    stage -= ApparatusSize;

    const int ArmorSize(mReferencables.getArmors().getSize());

    if (stage < ArmorSize)
    {
        armorCheck(stage, mReferencables.getArmors(), messages);
        return;
    }

    stage -= ArmorSize;

    const int ClothingSize(mReferencables.getClothing().getSize());

    if (stage < ClothingSize)
    {
        clothingCheck(stage, mReferencables.getClothing(), messages);
        return;
    }

    stage -= ClothingSize;

    const int ContainerSize(mReferencables.getContainers().getSize());

    if (stage < ContainerSize)
    {
        containerCheck(stage, mReferencables.getContainers(), messages);
        return;
    }

    stage -= ContainerSize;

    const int DoorSize(mReferencables.getDoors().getSize());

    if (stage < DoorSize)
    {
        doorCheck(stage, mReferencables.getDoors(), messages);
        return;
    }

    stage -= DoorSize;

    const int IngredientSize(mReferencables.getIngredients().getSize());

    if (stage < IngredientSize)
    {
        ingredientCheck(stage, mReferencables.getIngredients(), messages);
        return;
    }

    stage -= IngredientSize;

    const int CreatureLevListSize(mReferencables.getCreatureLevelledLists().getSize());

    if (stage < CreatureLevListSize)
    {
        creaturesLevListCheck(stage, mReferencables.getCreatureLevelledLists(), messages);
        return;
    }

    stage -= CreatureLevListSize;

    const int ItemLevelledListSize(mReferencables.getItemLevelledList().getSize());

    if (stage < ItemLevelledListSize)
    {
        itemLevelledListCheck(stage, mReferencables.getItemLevelledList(), messages);
        return;
    }

    stage -= ItemLevelledListSize;

    const int LightSize(mReferencables.getLights().getSize());

    if (stage < LightSize)
    {
        lightCheck(stage, mReferencables.getLights(), messages);
        return;
    }

    stage -= LightSize;

    const int LockpickSize(mReferencables.getLocpicks().getSize());

    if (stage < LockpickSize)
    {
        lockpickCheck(stage, mReferencables.getLocpicks(), messages);
        return;
    }

    stage -= LockpickSize;

    const int MiscSize(mReferencables.getMiscellaneous().getSize());

    if (stage < MiscSize)
    {
        miscCheck(stage, mReferencables.getMiscellaneous(), messages);
        return;
    }

    stage -= MiscSize;

    const int NPCSize(mReferencables.getNPCs().getSize());

    if (stage < NPCSize)
    {
        npcCheck(stage, mReferencables.getNPCs(), messages);
        return;
    }
}

int CSMTools::ReferenceableCheckStage::setup()
{
    return mReferencables.getSize();
}

void CSMTools::ReferenceableCheckStage::bookCheck(int stage, const CSMWorld::RefIdDataContainer< ESM::Book >& records, std::vector< std::string >& messages)
{
    const CSMWorld::RecordBase& baserecord = records.getRecord(stage);

    if (baserecord.isDeleted())
    {
        return;
    }

    const ESM::Book& Book = (static_cast<const CSMWorld::Record<ESM::Book>& >(baserecord)).get();
    CSMWorld::UniversalId id(CSMWorld::UniversalId::Type_Book, Book.mId);

    //Checking for name
    if (Book.mName.empty())
    {
        messages.push_back(id.toString() + "|" + Book.mId + " has an empty name");
    }

    //Checking for weight
    if (Book.mData.mWeight < 0)
    {
        messages.push_back(id.toString() + "|" + Book.mId + " has negative weight");
    }

    //Checking for value
    if (Book.mData.mValue < 0)
    {
        messages.push_back(id.toString() + "|" + Book.mId + " has negative value");
    }

//checking for model
    if (Book.mModel.empty())
    {
        messages.push_back(id.toString() + "|" + Book.mId + " has no model");
    }

    //checking for icon
    if (Book.mIcon.empty())
    {
        messages.push_back(id.toString() + "|" + Book.mId + " has no icon");
    }

    //checking for enchantment points
    if (Book.mData.mEnchant < 0)
    {
        messages.push_back(id.toString() + "|" + Book.mId + " has negative enchantment");
    }
}

void CSMTools::ReferenceableCheckStage::activatorCheck(int stage, const CSMWorld::RefIdDataContainer< ESM::Activator >& records, std::vector< std::string >& messages)
{
    const CSMWorld::RecordBase& baserecord = records.getRecord(stage);

    if (baserecord.isDeleted())
    {
        return;
    }

    const ESM::Activator& Activator = (static_cast<const CSMWorld::Record<ESM::Activator>& >(baserecord)).get();
    CSMWorld::UniversalId id(CSMWorld::UniversalId::Type_Activator, Activator.mId);

    //Checking for model, IIRC all activators should have a model
    if (Activator.mModel.empty())
    {
        messages.push_back(id.toString() + "|" + Activator.mId + " has no model");
    }
}

void CSMTools::ReferenceableCheckStage::potionCheck(int stage, const CSMWorld::RefIdDataContainer< ESM::Potion >& records, std::vector< std::string >& messages)
{
    const CSMWorld::RecordBase& baserecord = records.getRecord(stage);

    if (baserecord.isDeleted())
    {
        return;
    }

    const ESM::Potion& Potion = (static_cast<const CSMWorld::Record<ESM::Potion>& >(baserecord)).get();
    CSMWorld::UniversalId id(CSMWorld::UniversalId::Type_Potion, Potion.mId);

    //Checking for name
    if (Potion.mName.empty())
    {
        messages.push_back(id.toString() + "|" + Potion.mId + " has an empty name");
    }

    //Checking for weight
    if (Potion.mData.mWeight < 0)
    {
        messages.push_back(id.toString() + "|" + Potion.mId + " has negative weight");
    }

    //Checking for value
    if (Potion.mData.mValue < 0)
    {
        messages.push_back(id.toString() + "|" + Potion.mId + " has negative value");
    }

//checking for model
    if (Potion.mModel.empty())
    {
        messages.push_back(id.toString() + "|" + Potion.mId + " has no model");
    }

    //checking for icon
    if (Potion.mIcon.empty())
    {
        messages.push_back(id.toString() + "|" + Potion.mId + " has no icon");
    }

    //IIRC potion can have empty effects list just fine.
}


void CSMTools::ReferenceableCheckStage::apparatusCheck(int stage, const CSMWorld::RefIdDataContainer< ESM::Apparatus >& records, std::vector< std::string >& messages)
{
    const CSMWorld::RecordBase& baserecord = records.getRecord(stage);

    if (baserecord.isDeleted())
    {
        return;
    }

    const ESM::Apparatus& Apparatus = (static_cast<const CSMWorld::Record<ESM::Apparatus>& >(baserecord)).get();
    CSMWorld::UniversalId id(CSMWorld::UniversalId::Type_Apparatus, Apparatus.mId);

    //Checking for name
    if (Apparatus.mName.empty())
    {
        messages.push_back(id.toString() + "|" + Apparatus.mId + " has an empty name");
    }

    //Checking for weight
    if (Apparatus.mData.mWeight < 0)
    {
        messages.push_back(id.toString() + "|" + Apparatus.mId + " has negative weight");
    }

    //Checking for value
    if (Apparatus.mData.mValue < 0)
    {
        messages.push_back(id.toString() + "|" + Apparatus.mId + " has negative value");
    }

//checking for model
    if (Apparatus.mModel.empty())
    {
        messages.push_back(id.toString() + "|" + Apparatus.mId + " has no model");
    }

    //checking for icon
    if (Apparatus.mIcon.empty())
    {
        messages.push_back(id.toString() + "|" + Apparatus.mId + " has no icon");
    }

    //checking for quality, 0 → apparatus is basicly useless, any negative → apparatus is harmfull instead of helpfull
    if (Apparatus.mData.mQuality <= 0)
    {
        messages.push_back(id.toString() + "|" + Apparatus.mId + " has non-positive quality");
    }
}

void CSMTools::ReferenceableCheckStage::armorCheck(int stage, const CSMWorld::RefIdDataContainer< ESM::Armor >& records, std::vector< std::string >& messages)
{
    const CSMWorld::RecordBase& baserecord = records.getRecord(stage);

    if (baserecord.isDeleted())
    {
        return;
    }

    const ESM::Armor& Armor = (static_cast<const CSMWorld::Record<ESM::Armor>& >(baserecord)).get();
    CSMWorld::UniversalId id(CSMWorld::UniversalId::Type_Armor, Armor.mId);

    //Checking for name
    if (Armor.mName.empty())
    {
        messages.push_back(id.toString() + "|" + Armor.mId + " has an empty name");
    }

    //Checking for weight
    if (Armor.mData.mWeight < 0)
    {
        messages.push_back(id.toString() + "|" + Armor.mId + " has negative weight");
    }

    //Checking for value
    if (Armor.mData.mValue < 0)
    {
        messages.push_back(id.toString() + "|" + Armor.mId + " has negative value");
    }

//checking for model
    if (Armor.mModel.empty())
    {
        messages.push_back(id.toString() + "|" + Armor.mId + " has no model");
    }

    //checking for icon
    if (Armor.mIcon.empty())
    {
        messages.push_back(id.toString() + "|" + Armor.mId + " has no icon");
    }

    //checking for enchantment points
    if (Armor.mData.mEnchant < 0)
    {
        messages.push_back(id.toString() + "|" + Armor.mId + " has negative enchantment");
    }

    //checking for armor class, armor should have poistive armor class, but 0 is considered legal
    if (Armor.mData.mArmor < 0)
    {
        messages.push_back(id.toString() + "|" + Armor.mId + " has negative armor class");
    }

    //checking for health. Only positive numbers are allowed, and 0 is illegal
    if (Armor.mData.mHealth <= 0)
    {
        messages.push_back(id.toString() + "|" + Armor.mId + " has non positive health");
    }
}

void CSMTools::ReferenceableCheckStage::clothingCheck(int stage, const CSMWorld::RefIdDataContainer< ESM::Clothing >& records, std::vector< std::string >& messages)
{
    const CSMWorld::RecordBase& baserecord = records.getRecord(stage);

    if (baserecord.isDeleted())
    {
        return;
    }

    const ESM::Clothing& Clothing = (static_cast<const CSMWorld::Record<ESM::Clothing>& >(baserecord)).get();
    CSMWorld::UniversalId id(CSMWorld::UniversalId::Type_Clothing, Clothing.mId);

    //Checking for name
    if (Clothing.mName.empty())
    {
        messages.push_back(id.toString() + "|" + Clothing.mId + " has an empty name");
    }

    //Checking for weight
    if (Clothing.mData.mWeight < 0)
    {
        messages.push_back(id.toString() + "|" + Clothing.mId + " has negative weight");
    }

    //Checking for value
    if (Clothing.mData.mValue < 0)
    {
        messages.push_back(id.toString() + "|" + Clothing.mId + " has negative value");
    }

//checking for model
    if (Clothing.mModel.empty())
    {
        messages.push_back(id.toString() + "|" + Clothing.mId + " has no model");
    }

    //checking for icon
    if (Clothing.mIcon.empty())
    {
        messages.push_back(id.toString() + "|" + Clothing.mId + " has no icon");
    }

    //checking for enchantment points
    if (Clothing.mData.mEnchant < 0)
    {
        messages.push_back(id.toString() + "|" + Clothing.mId + " has negative enchantment");
    }
}

void CSMTools::ReferenceableCheckStage::containerCheck(int stage, const CSMWorld::RefIdDataContainer< ESM::Container >& records, std::vector< std::string >& messages)
{
    const CSMWorld::RecordBase& baserecord = records.getRecord(stage);

    if (baserecord.isDeleted())
    {
        return;
    }

    const ESM::Container& Container = (static_cast<const CSMWorld::Record<ESM::Container>& >(baserecord)).get();
    CSMWorld::UniversalId id(CSMWorld::UniversalId::Type_Container, Container.mId);

    //Checking for model, IIRC all containers should have a model
    if (Container.mModel.empty())
    {
        messages.push_back(id.toString() + "|" + Container.mId + " has no model");
    }

    //Checking for capacity (weight)
    if (Container.mWeight < 0) //0 is allowed
    {
        messages.push_back(id.toString() + "|" + Container.mId + " has negative weight (capacity)");
    }

    //checking for name
    if (Container.mName.empty())
    {
        messages.push_back(id.toString() + "|" + Container.mId + " has an empty name");
    }
}

void CSMTools::ReferenceableCheckStage::creatureCheck(int stage, const CSMWorld::RefIdDataContainer< ESM::Creature >& records, std::vector< std::string >& messages)
{
    const CSMWorld::RecordBase& baserecord = records.getRecord(stage);

    if (baserecord.isDeleted())
    {
        return;
    }

    const ESM::Creature& Creature = (static_cast<const CSMWorld::Record<ESM::Creature>&>(baserecord)).get();
    CSMWorld::UniversalId id(CSMWorld::UniversalId::Type_Creature, Creature.mId);

    if (Creature.mModel.empty())
    {
        messages.push_back(id.toString() + "|" + Creature.mId + " has no model");
    }

    if (Creature.mName.empty())
    {
        messages.push_back(id.toString() + "|" + Creature.mId + " has an empty name");
    }

    //stats checks
    if (Creature.mData.mLevel < 1)
    {
        messages.push_back(id.toString() + "|" + Creature.mId + " has non-postive level");
    }

    if (Creature.mData.mStrength < 0)
    {
        messages.push_back(id.toString() + "|" + Creature.mId + " has negative strength");
    }

    if (Creature.mData.mIntelligence < 0)
    {
        messages.push_back(id.toString() + "|" + Creature.mId + " has negative intelligence");
    }

    if (Creature.mData.mWillpower < 0)
    {
        messages.push_back(id.toString() + "|" + Creature.mId + " has negative willpower");
    }

    if (Creature.mData.mAgility < 0)
    {
        messages.push_back(id.toString() + "|" + Creature.mId + " has negative agility");
    }

    if (Creature.mData.mSpeed < 0)
    {
        messages.push_back(id.toString() + "|" + Creature.mId + " has negative speed");
    }

    if (Creature.mData.mEndurance < 0)
    {
        messages.push_back(id.toString() + "|" + Creature.mId + " has negative endurance");
    }

    if (Creature.mData.mPersonality < 0)
    {
        messages.push_back(id.toString() + "|" + Creature.mId + " has negative personality");
    }

    if (Creature.mData.mLuck < 0)
    {
        messages.push_back(id.toString() + "|" + Creature.mId + " has negative luck");
    }

    if (Creature.mData.mHealth < 0)
    {
        messages.push_back(id.toString() + "|" + Creature.mId + " has negative health");
    }

    if (Creature.mData.mSoul < 0)
    {
        messages.push_back(id.toString() + "|" + Creature.mId + " has negative soul value");
    }

    for (int i = 0; i < 6; ++i)
    {
        if (Creature.mData.mAttack[i] < 0)
        {
            messages.push_back(id.toString() + "|" + Creature.mId + " has negative attack strength");
            break;
        }
    }

    //TODO, find meaning of other values
    if (Creature.mData.mGold < 0) //It seems that this is for gold in merchant creatures
    {
        messages.push_back(id.toString() + "|" + Creature.mId + " has negative gold ");
    }
}

void CSMTools::ReferenceableCheckStage::doorCheck(int stage, const CSMWorld::RefIdDataContainer< ESM::Door >& records, std::vector< std::string >& messages)
{
    const CSMWorld::RecordBase& baserecord = records.getRecord(stage);

    if (baserecord.isDeleted())
    {
        return;
    }

    const ESM::Door& Door = (static_cast<const CSMWorld::Record<ESM::Door>&>(baserecord)).get();
    CSMWorld::UniversalId id(CSMWorld::UniversalId::Type_Door, Door.mId);

    //usual, name and model
    if (Door.mName.empty())
    {
        messages.push_back(id.toString() + "|" + Door.mId + " has an empty name");
    }

    if (Door.mModel.empty())
    {
        messages.push_back(id.toString() + "|" + Door.mId + " has no model");
    }

    //TODO, check what static unsigned int sRecordId; is for
}

void CSMTools::ReferenceableCheckStage::ingredientCheck(int stage, const CSMWorld::RefIdDataContainer< ESM::Ingredient >& records, std::vector< std::string >& messages)
{
    const CSMWorld::RecordBase& baserecord = records.getRecord(stage);

    if (baserecord.isDeleted())
    {
        return;
    }

    const ESM::Ingredient& Ingredient = (static_cast<const CSMWorld::Record<ESM::Ingredient>& >(baserecord)).get();
    CSMWorld::UniversalId id(CSMWorld::UniversalId::Type_Ingredient, Ingredient.mId);

    //Checking for name
    if (Ingredient.mName.empty())
    {
        messages.push_back(id.toString() + "|" + Ingredient.mId + " has an empty name");
    }

    //Checking for weight
    if (Ingredient.mData.mWeight < 0)
    {
        messages.push_back(id.toString() + "|" + Ingredient.mId + " has negative weight");
    }

    //Checking for value
    if (Ingredient.mData.mValue < 0)
    {
        messages.push_back(id.toString() + "|" + Ingredient.mId + " has negative value");
    }

//checking for model
    if (Ingredient.mModel.empty())
    {
        messages.push_back(id.toString() + "|" + Ingredient.mId + " has no model");
    }

    //checking for icon
    if (Ingredient.mIcon.empty())
    {
        messages.push_back(id.toString() + "|" + Ingredient.mId + " has no icon");
    }
}

void CSMTools::ReferenceableCheckStage::creaturesLevListCheck(int stage, const CSMWorld::RefIdDataContainer< ESM::CreatureLevList >& records, std::vector< std::string >& messages)
{
    const CSMWorld::RecordBase& baserecord = records.getRecord(stage);

    if (baserecord.isDeleted())
    {
        return;
    }

    const ESM::CreatureLevList& CreatureLevList = (static_cast<const CSMWorld::Record<ESM::CreatureLevList>& >(baserecord)).get();
    CSMWorld::UniversalId id(CSMWorld::UniversalId::Type_CreatureLevelledList, CreatureLevList.mId); //CreatureLevList but Type_CreatureLevelledList :/

    for (unsigned i = 0; i < CreatureLevList.mList.size(); ++i)
    {
        if (CreatureLevList.mList[i].mId.empty())
        {
            messages.push_back(id.toString() + "|" + CreatureLevList.mId + " contains item with empty Id");
        }

        if (CreatureLevList.mList[i].mLevel < 1)
        {
            messages.push_back(id.toString() + "|" + CreatureLevList.mId + " contains item with non-positive level");
        }
    }
}

void CSMTools::ReferenceableCheckStage::itemLevelledListCheck(int stage, const CSMWorld::RefIdDataContainer< ESM::ItemLevList >& records, std::vector< std::string >& messages)
{
    const CSMWorld::RecordBase& baserecord = records.getRecord(stage);

    if (baserecord.isDeleted())
    {
        return;
    }

    const ESM::ItemLevList& ItemLevList = (static_cast<const CSMWorld::Record<ESM::ItemLevList>& >(baserecord)).get();
    CSMWorld::UniversalId id(CSMWorld::UniversalId::Type_ItemLevelledList, ItemLevList.mId);

    for (unsigned i = 0; i < ItemLevList.mList.size(); ++i)
    {
        if (ItemLevList.mList[i].mId.empty())
        {
            messages.push_back(id.toString() + "|" + ItemLevList.mId + " contains item with empty Id");
        }

        if (ItemLevList.mList[i].mLevel < 1)
        {
            messages.push_back(id.toString() + "|" + ItemLevList.mId + " contains item with non-positive level");
        }
    }
}

void CSMTools::ReferenceableCheckStage::lightCheck(int stage, const CSMWorld::RefIdDataContainer< ESM::Light >& records, std::vector< std::string >& messages)
{
    const CSMWorld::RecordBase& baserecord = records.getRecord(stage);

    if (baserecord.isDeleted())
    {
        return;
    }

    const ESM::Light& Light = (static_cast<const CSMWorld::Record<ESM::Light>& >(baserecord)).get();
    CSMWorld::UniversalId id(CSMWorld::UniversalId::Type_Light, Light.mId);

    if (Light.mData.mRadius < 0)
    {
        messages.push_back(id.toString() + "|" + Light.mId + " has negative light radius");
    }

    if (Light.mData.mFlags & ESM::Light::Carry)
    {
        if (Light.mIcon.empty()) //Needs to be checked with carrable flag
        {
            messages.push_back(id.toString() + "|" + Light.mId + " has no icon");
        }

        if (Light.mData.mWeight < 0)
        {
            messages.push_back(id.toString() + "|" + Light.mId + " has negative weight");
        }

        if (Light.mData.mValue < 0)
        {
            messages.push_back(id.toString() + "|" + Light.mId + " has negative value");
        }

        if (Light.mModel.empty())
        {
            messages.push_back(id.toString() + "|" + Light.mId + " has no model");
        }

        if (Light.mData.mTime == 0)
        {
            messages.push_back(id.toString() + "|" + Light.mId + " has zero duration");
        }
    }
}

void CSMTools::ReferenceableCheckStage::lockpickCheck(int stage, const CSMWorld::RefIdDataContainer< ESM::Lockpick >& records, std::vector< std::string >& messages)
{
    const CSMWorld::RecordBase& baserecord = records.getRecord(stage);

    if (baserecord.isDeleted())
    {
        return;
    }

    const ESM::Lockpick& Lockpick = (static_cast<const CSMWorld::Record<ESM::Lockpick>& >(baserecord)).get();
    CSMWorld::UniversalId id(CSMWorld::UniversalId::Type_Lockpick, Lockpick.mId);

    //Checking for name
    if (Lockpick.mName.empty())
    {
        messages.push_back(id.toString() + "|" + Lockpick.mId + " has an empty name");
    }

    //Checking for weight
    if (Lockpick.mData.mWeight < 0)
    {
        messages.push_back(id.toString() + "|" + Lockpick.mId + " has negative weight");
    }

    //Checking for value
    if (Lockpick.mData.mValue < 0)
    {
        messages.push_back(id.toString() + "|" + Lockpick.mId + " has negative value");
    }

//checking for model
    if (Lockpick.mModel.empty())
    {
        messages.push_back(id.toString() + "|" + Lockpick.mId + " has no model");
    }

    //checking for icon
    if (Lockpick.mIcon.empty())
    {
        messages.push_back(id.toString() + "|" + Lockpick.mId + " has no icon");
    }

    if (Lockpick.mData.mQuality <= 0)
    {
        messages.push_back(id.toString() + "|" + Lockpick.mId + " has non-positive quality");
    }

    if (Lockpick.mData.mUses <= 0)
    {
        messages.push_back(id.toString() + "|" + Lockpick.mId + " has no uses left");
    }
}

void CSMTools::ReferenceableCheckStage::miscCheck(int stage, const CSMWorld::RefIdDataContainer< ESM::Miscellaneous >& records, std::vector< std::string >& messages)
{
    const CSMWorld::RecordBase& baserecord = records.getRecord(stage);

    if (baserecord.isDeleted())
    {
        return;
    }

    const ESM::Miscellaneous& Miscellaneous = (static_cast<const CSMWorld::Record<ESM::Miscellaneous>& >(baserecord)).get();
    CSMWorld::UniversalId id(CSMWorld::UniversalId::Type_Miscellaneous, Miscellaneous.mId);

    //Checking for name
    if (Miscellaneous.mName.empty())
    {
        messages.push_back(id.toString() + "|" + Miscellaneous.mId + " has an empty name");
    }

    //Checking for weight
    if (Miscellaneous.mData.mWeight < 0)
    {
        messages.push_back(id.toString() + "|" + Miscellaneous.mId + " has negative weight");
    }

    //Checking for value
    if (Miscellaneous.mData.mValue < 0)
    {
        messages.push_back(id.toString() + "|" + Miscellaneous.mId + " has negative value");
    }

//checking for model
    if (Miscellaneous.mModel.empty())
    {
        messages.push_back(id.toString() + "|" + Miscellaneous.mId + " has no model");
    }

    //checking for icon
    if (Miscellaneous.mIcon.empty())
    {
        messages.push_back(id.toString() + "|" + Miscellaneous.mId + " has no icon");
    }
}

void CSMTools::ReferenceableCheckStage::npcCheck(int stage, const CSMWorld::RefIdDataContainer< ESM::NPC >& records, std::vector< std::string >& messages)
{
    const CSMWorld::RecordBase& baserecord = records.getRecord(stage);

    if (baserecord.isDeleted())
    {
        return;
    }

    const ESM::NPC& NPC = (static_cast<const CSMWorld::Record<ESM::NPC>& >(baserecord)).get();
    CSMWorld::UniversalId id(CSMWorld::UniversalId::Type_Npc, NPC.mId);



    short level(NPC.mNpdt52.mLevel);
    char Disposition(NPC.mNpdt52.mDisposition);
    char Reputation(NPC.mNpdt52.mReputation);
    char Rank(NPC.mNpdt52.mRank);
    //Don't know what unknown is for
    int Gold(NPC.mNpdt52.mGold);

    if (NPC.mNpdtType == 12) //12 = autocalculated
    {
        if (! NPC.mFlags & 0x0008) //0x0008 = autocalculated flag
        {
            messages.push_back(id.toString() + "|" + NPC.mId + " mNpdtType and flags mismatch!"); //should not happend?
            return;
        }

        level = NPC.mNpdt12.mLevel;
        Disposition = NPC.mNpdt12.mDisposition;
        Reputation = NPC.mNpdt12.mReputation;
        Rank = NPC.mNpdt12.mRank;
        Gold = NPC.mNpdt12.mGold;
    }
    else
    {
        if (NPC.mNpdt52.mMana < 0)
        {
            messages.push_back(id.toString() + "|" + NPC.mId + " mana has negative value");
        }

        if (NPC.mNpdt52.mFatigue < 0)
        {
            messages.push_back(id.toString() + "|" + NPC.mId + " fatigue has negative value");
        }

        if (NPC.mNpdt52.mAgility == 0)
        {
            messages.push_back(id.toString() + "|" + NPC.mId + " agility has zero value");
        }

        if (NPC.mNpdt52.mEndurance == 0)
        {
            messages.push_back(id.toString() + "|" + NPC.mId + " endurance has zero value");
        }

        if (NPC.mNpdt52.mIntelligence == 0)
        {
            messages.push_back(id.toString() + "|" + NPC.mId + " intelligence has zero value");
        }

        if (NPC.mNpdt52.mLuck == 0)
        {
            messages.push_back(id.toString() + "|" + NPC.mId + " luck has zero value");
        }

        if (NPC.mNpdt52.mPersonality == 0)
        {
            messages.push_back(id.toString() + "|" + NPC.mId + " personality has zero value");
        }

        if (NPC.mNpdt52.mStrength == 0)
        {
            messages.push_back(id.toString() + "|" + NPC.mId + " strength has zero value");
        }

        if (NPC.mNpdt52.mSpeed == 0)
        {
            messages.push_back(id.toString() + "|" + NPC.mId + " speed has zero value");
        }

        if (NPC.mNpdt52.mWillpower == 0)
        {
            messages.push_back(id.toString() + "|" + NPC.mId + " willpower has zero value");
        }

    }

    if (level < 1)
    {
        messages.push_back(id.toString() + "|" + NPC.mId + " level is non positive");
    }

    if (Gold < 0)
    {
        messages.push_back(id.toString() + "|" + NPC.mId + " gold has negative value");
    }

    if (NPC.mName.empty())
    {
        messages.push_back(id.toString() + "|" + NPC.mId + " has any empty name");
    }

    if (NPC.mClass.empty())
    {
        messages.push_back(id.toString() + "|" + NPC.mId + " has any empty class");
    }
    else //checking if there is such class
    {
        bool nosuchclass(true);

        for (int i = 0; i < mClasses.getSize(); ++i)
        {
            if (mClasses.getRecord(i).get().mName == NPC.mClass)
            {
                nosuchclass = false;
                break;
            }
        }

        if (nosuchclass)
        {
            messages.push_back(id.toString() + "|" + NPC.mId + " has invalid class");
        }
    }

    if (NPC.mRace.empty())
    {
        messages.push_back(id.toString() + "|" + NPC.mId + " has any empty race");
    }
    else //checking if there is a such race
    {
        bool nosuchrace(true);

        for (int i = 0; i < mRaces.getSize(); ++i)
        {
            if (mRaces.getRecord(i).get().mName == NPC.mRace)
            {
                nosuchrace = false;
                break;
            }
        }

        if (nosuchrace)
        {
            messages.push_back(id.toString() + "|" + NPC.mId + " has invalid race");
        }
    }

    //TODO: reputation, Disposition, rank, everything else
}
