#ifndef CSM_WOLRD_COLUMNS_H
#define CSM_WOLRD_COLUMNS_H

#include <string>
#include <vector>

namespace CSMWorld
{
    namespace Columns
    {
        enum ColumnId
        {
            ColumnId_Value = 0,
            ColumnId_Id = 1,
            ColumnId_Modification = 2,
            ColumnId_RecordType = 3,
            ColumnId_ValueType = 4,
            ColumnId_Description = 5,
            ColumnId_Specialisation = 6,
            ColumnId_Attribute = 7,
            ColumnId_Name = 8,
            ColumnId_Playable = 9,
            ColumnId_Hidden = 10,
            ColumnId_MaleWeight = 11,
            ColumnId_FemaleWeight = 12,
            ColumnId_MaleHeight = 13,
            ColumnId_FemaleHeight = 14,
            ColumnId_Volume = 15,
            ColumnId_MinRange = 16,
            ColumnId_MaxRange = 17,
            ColumnId_SoundFile = 18,
            ColumnId_MapColour = 19,
            ColumnId_SleepEncounter = 20,
            ColumnId_Texture = 21,
            ColumnId_SpellType = 22,
            ColumnId_Cost = 23,
            ColumnId_ScriptText = 24,
            ColumnId_Region = 25,
            ColumnId_Cell = 26,
            ColumnId_Scale = 27,
            ColumnId_Owner = 28,
            ColumnId_Soul = 29,
            ColumnId_Faction = 30,
            ColumnId_FactionIndex = 31,
            ColumnId_Charges = 32,
            ColumnId_Enchantment = 33,
            ColumnId_CoinValue = 34,
            ColumnId_Teleport = 25,
            ColumnId_TeleportCell = 26,
            ColumnId_LockLevel = 27,
            ColumnId_Key = 28,
            ColumnId_Trap = 29,
            ColumnId_BeastRace = 30,
            ColumnId_AutoCalc = 31,
            ColumnId_StarterSpell = 32,
            ColumnId_AlwaysSucceeds = 33,
            ColumnId_SleepForbidden = 34,
            ColumnId_InteriorWater = 35,
            ColumnId_InteriorSky = 36,
            ColumnId_Model = 37,
            ColumnId_Script = 38,
            ColumnId_Icon = 39,
            ColumnId_Weight = 40,
            ColumnId_EnchantmentPoints = 31,
            ColumnId_Quality = 32,
            ColumnId_Ai = 33,
            ColumnId_AiHello = 34,
            ColumnId_AiFlee = 35,
            ColumnId_AiFight = 36,
            ColumnId_AiAlarm = 37,
            ColumnId_BuysWeapons = 38,
            ColumnId_BuysArmor = 39,
            ColumnId_BuysClothing = 40,
            ColumnId_BuysBooks = 41,
            ColumnId_BuysIngredients = 42,
            ColumnId_BuysLockpicks = 43,
            ColumnId_BuysProbes = 44,
            ColumnId_BuysLights = 45,
            ColumnId_BuysApparati = 46,
            ColumnId_BuysRepairItems = 47,
            ColumnId_BuysMiscItems = 48,
            ColumnId_BuysPotions = 49,
            ColumnId_BuysMagicItems = 50,
            ColumnId_SellsSpells = 51,
            ColumnId_Trainer = 52,
            ColumnId_Spellmaking = 53,
            ColumnId_EnchantingService = 54,
            ColumnId_RepairService = 55,
            ColumnId_ApparatusType = 56,
            ColumnId_ArmorType = 57,
            ColumnId_Health = 58,
            ColumnId_ArmorValue = 59,
            ColumnId_Scroll = 60,
            ColumnId_ClothingType = 61,
            ColumnId_WeightCapacity = 62,
            ColumnId_OrganicContainer = 63,
            ColumnId_Respawn = 64,
            ColumnId_CreatureType = 65,
            ColumnId_SoulPoints = 66,
            ColumnId_OriginalCreature = 67,
            ColumnId_Biped = 68,
            ColumnId_HasWeapon = 69,
            ColumnId_NoMovement = 70,
            ColumnId_Swims = 71,
            ColumnId_Flies = 72,
            ColumnId_Walks = 73,
            ColumnId_Essential = 74,
            ColumnId_SkeletonBlood = 75,
            ColumnId_MetalBlood = 76,
            ColumnId_OpenSound = 77,
            ColumnId_CloseSound = 78,
            ColumnId_Duration = 79,
            ColumnId_Radius = 80,
            ColumnId_Colour = 81,
            ColumnId_Sound = 82,
            ColumnId_Dynamic = 83,
            ColumnId_Portable = 84,
            ColumnId_NegativeLight = 85,
            ColumnId_Flickering = 86,
            ColumnId_SlowFlickering = 87,
            ColumnId_Pulsing = 88,
            ColumnId_SlowPulsing = 89,
            ColumnId_Fire = 90,
            ColumnId_OffByDefault = 91,
            ColumnId_IsKey = 92,
            ColumnId_Race = 93,
            ColumnId_Class = 94,
            Columnid_Hair = 95,
            ColumnId_Head = 96,
            ColumnId_Female = 97,
            ColumnId_WeaponType = 98,
            ColumnId_WeaponSpeed = 99,
            ColumnId_WeaponReach = 100,
            ColumnId_MinChop = 101,
            ColumnId_MaxChip = 102,
            Columnid_MinSlash = 103,
            ColumnId_MaxSlash = 104,
            ColumnId_MinThrust = 105,
            ColumnId_MaxThrust = 106,
            ColumnId_Magical = 107,
            ColumnId_Silver = 108,
            ColumnId_Filter = 109,
            ColumnId_PositionXPos = 110,
            ColumnId_PositionYPos = 111,
            ColumnId_PositionZPos = 112,
            ColumnId_PositionXRot = 113,
            ColumnId_PositionYRot = 114,
            ColumnId_PositionZRot = 115,
            ColumnId_DoorPositionXPos = 116,
            ColumnId_DoorPositionYPos = 117,
            ColumnId_DoorPositionZPos = 118,
            ColumnId_DoorPositionXRot = 119,
            ColumnId_DoorPositionYRot = 120,
            ColumnId_DoorPositionZRot = 121,

            // Allocated to a separate value range, so we don't get a collision should we ever need
            // to extend the number of use values.
            ColumnId_UseValue1 = 0x10000,
            ColumnId_UseValue2 = 0x10001,
            ColumnId_UseValue3 = 0x10002,
            ColumnId_UseValue4 = 0x10003,

            // Allocated to a separate value range, so we don't get a collision should we ever need
            // to extend the number of attributes. Note that this is not the number of different
            // attributes, but the number of attributes that can be references from a record.
            ColumnId_Attribute1 = 0x20000,
            ColumnId_Attribute2 = 0x20001,

            // Allocated to a separate value range, so we don't get a collision should we ever need
            // to extend the number of skills. Note that this is not the number of different
            // skills, but the number of skills that can be references from a record.
            ColumnId_MajorSkill1 = 0x30000,
            ColumnId_MajorSkill2 = 0x30001,
            ColumnId_MajorSkill3 = 0x30002,
            ColumnId_MajorSkill4 = 0x30003,
            ColumnId_MajorSkill5 = 0x30004,

            ColumnId_MinorSkill1 = 0x40000,
            ColumnId_MinorSkill2 = 0x40001,
            ColumnId_MinorSkill3 = 0x40002,
            ColumnId_MinorSkill4 = 0x40003,
            ColumnId_MinorSkill5 = 0x40004,

            ColumnId_Skill1 = 0x50000,
            ColumnId_Skill2 = 0x50001,
            ColumnId_Skill3 = 0x50002,
            ColumnId_Skill4 = 0x50003,
            ColumnId_Skill5 = 0x50004,
            ColumnId_Skill6 = 0x50005
        };

        std::string getName (ColumnId column);

        int getId (const std::string& name);
        ///< Will return -1 for an invalid name.

        bool hasEnums (ColumnId column);

        std::vector<std::string> getEnums (ColumnId column);
        ///< Returns an empty vector, if \æ column isn't an enum type column.
    }
}

#endif
