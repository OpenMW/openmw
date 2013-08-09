
#include "columns.hpp"

#include <components/misc/stringops.hpp>

namespace CSMWorld
{
    namespace Columns
    {
        struct ColumnDesc
        {
            int mId;
            const char *mName;
        };

        const ColumnDesc sNames[] =
        {
            { ColumnId_Value, "Value" },
            { ColumnId_Id, "ID" },
            { ColumnId_Modification, "Modified" },
            { ColumnId_RecordType, "Record Type" },
            { ColumnId_ValueType, "Value Type" },
            { ColumnId_Description, "Description" },
            { ColumnId_Specialisation, "Specialisation" },
            { ColumnId_Attribute, "Attribute" },
            { ColumnId_Name, "Name" },
            { ColumnId_Playable, "Playable" },
            { ColumnId_Hidden, "Hidden" },
            { ColumnId_MaleWeight, "Male Weight" },
            { ColumnId_FemaleWeight, "Female Weight" },
            { ColumnId_MaleHeight, "Male Height" },
            { ColumnId_FemaleHeight, "Female Height" },
            { ColumnId_Volume, "Volume" },
            { ColumnId_MinRange, "Min Range" },
            { ColumnId_MaxRange, "Max Range" },
            { ColumnId_SoundFile, "Sound File" },
            { ColumnId_MapColour, "Map Colour" },
            { ColumnId_SleepEncounter, "Sleep Encounter" },
            { ColumnId_Texture, "Texture" },
            { ColumnId_SpellType, "Spell Type" },
            { ColumnId_Cost, "Cost" },
            { ColumnId_ScriptText, "Script Text" },
            { ColumnId_Region, "Region" },
            { ColumnId_Cell, "Cell" },
            { ColumnId_Scale, "Scale" },
            { ColumnId_Owner, "Owner" },
            { ColumnId_Soul, "Soul" },
            { ColumnId_Faction, "Faction" },
            { ColumnId_FactionIndex, "Faction Index" },
            { ColumnId_Charges, "Charges" },
            { ColumnId_Enchantment, "Enchantment" },
            { ColumnId_Value, "Coin Value" },
            { ColumnId_Teleport, "Teleport" },
            { ColumnId_TeleportCell, "Teleport Cell" },
            { ColumnId_LockLevel, "Lock Level" },
            { ColumnId_Key, "Key" },
            { ColumnId_Trap, "Trap" },
            { ColumnId_BeastRace, "Beast Race" },
            { ColumnId_AutoCalc, "Auto Calc" },
            { ColumnId_StarterSpell, "Starter Spell" },
            { ColumnId_AlwaysSucceeds, "Always Succeeds" },
            { ColumnId_SleepForbidden, "Sleep Forbidden" },
            { ColumnId_InteriorWater, "Interior Water" },
            { ColumnId_InteriorSky, "Interior Sky" },
            { ColumnId_Model, "Model" },
            { ColumnId_Script, "Script" },
            { ColumnId_Icon, "Icon" },
            { ColumnId_Weight, "Weight" },
            { ColumnId_EnchantmentPoints, "Enchantment Points" },
            { ColumnId_Quality, "Quality" },
            { ColumnId_Ai, "AI" },
            { ColumnId_AiHello, "AI Hello" },
            { ColumnId_AiFlee, "AI Flee" },
            { ColumnId_AiFight, "AI Fight" },
            { ColumnId_AiAlarm, "AI Alarm" },
            { ColumnId_BuysWeapons, "Buys Weapons" },
            { ColumnId_BuysArmor, "Buys Armor" },
            { ColumnId_BuysClothing, "Buys Clothing" },
            { ColumnId_BuysBooks, "Buys Books" },
            { ColumnId_BuysIngredients, "Buys Ingredients" },
            { ColumnId_BuysLockpicks, "Buys Lockpicks" },
            { ColumnId_BuysProbes, "Buys Probes" },
            { ColumnId_BuysLights, "Buys Lights" },
            { ColumnId_BuysApparati, "Buys Apparati" },
            { ColumnId_BuysRepairItems, "Buys Repair Items" },
            { ColumnId_BuysMiscItems, "Buys Misc Items" },
            { ColumnId_BuysPotions, "Buys Potions" },
            { ColumnId_BuysMagicItems, "Buys Magic Items" },
            { ColumnId_SellsSpells, "Sells Spells" },
            { ColumnId_Trainer, "Trainer" },
            { ColumnId_Spellmaking, "Spellmaking" },
            { ColumnId_EnchantingService, "Enchanting Service" },
            { ColumnId_RepairService, "Repair Serivce" },
            { ColumnId_ApparatusType, "Apparatus Type" },
            { ColumnId_ArmorType, "Armor Type" },
            { ColumnId_Health, "Health" },
            { ColumnId_ArmorValue, "Armor Value" },
            { ColumnId_Scroll, "Scroll" },
            { ColumnId_ClothingType, "Clothing Type" },
            { ColumnId_WeightCapacity, "Weight Capacity" },
            { ColumnId_OrganicContainer, "Organic Container" },
            { ColumnId_Respawn, "Respawn" },
            { ColumnId_CreatureType, "Creature Type" },
            { ColumnId_SoulPoints, "Soul Points" },
            { ColumnId_OriginalCreature, "Original Creature" },
            { ColumnId_Biped, "Biped" },
            { ColumnId_HasWeapon, "Has Weapon" },
            { ColumnId_NoMovement, "No Movement" },
            { ColumnId_Swims, "Swims" },
            { ColumnId_Flies, "Flies" },
            { ColumnId_Walks, "Walks" },
            { ColumnId_Essential, "Essential" },
            { ColumnId_SkeletonBlood, "Skeleton Blood" },
            { ColumnId_MetalBlood, "Metal Blood" },
            { ColumnId_OpenSound, "Open Sound" },
            { ColumnId_CloseSound, "Close Sound" },
            { ColumnId_Duration, "Duration" },
            { ColumnId_Radius, "Radius" },
            { ColumnId_Colour, "Colour" },
            { ColumnId_Sound, "Sound" },
            { ColumnId_Dynamic, "Dynamic" },
            { ColumnId_Portable, "Portable" },
            { ColumnId_NegativeLight, "Negative Light" },
            { ColumnId_Flickering, "Flickering" },
            { ColumnId_SlowFlickering, "Slow Flickering" },
            { ColumnId_Pulsing, "Pulsing" },
            { ColumnId_SlowPulsing, "Slow Pulsing" },
            { ColumnId_Fire, "Fire" },
            { ColumnId_OffByDefault, "Off by default" },
            { ColumnId_IsKey, "Is Key" },
            { ColumnId_Race, "Race" },
            { ColumnId_Class, "Class" },
            { Columnid_Hair, "Hair" },
            { ColumnId_Head, "Head" },
            { ColumnId_Female, "Female" },
            { ColumnId_WeaponType, "Weapon Type" },
            { ColumnId_WeaponSpeed, "Weapon Speed" },
            { ColumnId_WeaponReach, "Weapon Reach" },
            { ColumnId_MinChop, "Min Chop" },
            { ColumnId_MaxChip, "Max Chop" },
            { Columnid_MinSlash, "Min Slash" },
            { ColumnId_MaxSlash, "Max Slash" },
            { ColumnId_MinThrust, "Min Thrust" },
            { ColumnId_MaxThrust, "Max Thrust" },
            { ColumnId_Magical, "Magical" },
            { ColumnId_Silver, "Silver" },

            { ColumnId_UseValue1, "Use value 1" },
            { ColumnId_UseValue2, "Use value 2" },
            { ColumnId_UseValue3, "Use value 3" },
            { ColumnId_UseValue4, "Use value 4" },

            { ColumnId_Attribute1, "Attribute 1" },
            { ColumnId_Attribute2, "Attribute 2" },

            { ColumnId_MajorSkill1, "Major Skill 1" },
            { ColumnId_MajorSkill2, "Major Skill 2" },
            { ColumnId_MajorSkill3, "Major Skill 3" },
            { ColumnId_MajorSkill4, "Major Skill 4" },
            { ColumnId_MajorSkill5, "Major Skill 5" },

            { ColumnId_MinorSkill1, "Minor Skill 1" },
            { ColumnId_MinorSkill2, "Minor Skill 2" },
            { ColumnId_MinorSkill3, "Minor Skill 3" },
            { ColumnId_MinorSkill4, "Minor Skill 4" },
            { ColumnId_MinorSkill5, "Minor Skill 5" },

            { ColumnId_Skill1, "Skill 1" },
            { ColumnId_Skill1, "Skill 2" },
            { ColumnId_Skill1, "Skill 3" },
            { ColumnId_Skill1, "Skill 4" },
            { ColumnId_Skill1, "Skill 5" },

            { -1, 0 } // end marker
        };
    }
}

std::string CSMWorld::Columns::getName (ColumnId column)
{
    for (int i=0; sNames[i].mName; ++i)
        if (column==sNames[i].mId)
            return sNames[i].mName;

    return "";
}

int CSMWorld::Columns::getId (const std::string& name)
{
    std::string name2 = Misc::StringUtils::lowerCase (name);

    for (int i=0; sNames[i].mName; ++i)
        if (name2==Misc::StringUtils::lowerCase (sNames[i].mName))
            return sNames[i].mId;

    return -1;
}