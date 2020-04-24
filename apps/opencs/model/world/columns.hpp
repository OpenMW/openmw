#ifndef CSM_WOLRD_COLUMNS_H
#define CSM_WOLRD_COLUMNS_H

#include <string>
#include <vector>

#include "columnbase.hpp"

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
            ColumnId_Teleport = 35,
            ColumnId_TeleportCell = 36,
            ColumnId_LockLevel = 37,
            ColumnId_Key = 38,
            ColumnId_Trap = 39,
            ColumnId_BeastRace = 40,
            ColumnId_AutoCalc = 41,
            ColumnId_StarterSpell = 42,
            ColumnId_AlwaysSucceeds = 43,
            ColumnId_SleepForbidden = 44,
            ColumnId_InteriorWater = 45,
            ColumnId_InteriorSky = 46,
            ColumnId_Model = 47,
            ColumnId_Script = 48,
            ColumnId_Icon = 49,
            ColumnId_Weight = 50,
            ColumnId_EnchantmentPoints = 51,
            ColumnId_Quality = 52,
            // unused
            ColumnId_AiHello = 54,
            ColumnId_AiFlee = 55,
            ColumnId_AiFight = 56,
            ColumnId_AiAlarm = 57,
            ColumnId_BuysWeapons = 58,
            ColumnId_BuysArmor = 59,
            ColumnId_BuysClothing = 60,
            ColumnId_BuysBooks = 61,
            ColumnId_BuysIngredients = 62,
            ColumnId_BuysLockpicks = 63,
            ColumnId_BuysProbes = 64,
            ColumnId_BuysLights = 65,
            ColumnId_BuysApparati = 66,
            ColumnId_BuysRepairItems = 67,
            ColumnId_BuysMiscItems = 68,
            ColumnId_BuysPotions = 69,
            ColumnId_BuysMagicItems = 70,
            ColumnId_SellsSpells = 71,
            ColumnId_Trainer = 72,
            ColumnId_Spellmaking = 73,
            ColumnId_EnchantingService = 74,
            ColumnId_RepairService = 75,
            ColumnId_ApparatusType = 76,
            ColumnId_ArmorType = 77,
            ColumnId_Health = 78,
            ColumnId_ArmorValue = 79,
            ColumnId_BookType = 80,
            ColumnId_ClothingType = 81,
            ColumnId_WeightCapacity = 82,
            ColumnId_OrganicContainer = 83,
            ColumnId_Respawn = 84,
            ColumnId_CreatureType = 85,
            ColumnId_SoulPoints = 86,
            ColumnId_ParentCreature = 87,
            ColumnId_Biped = 88,
            ColumnId_HasWeapon = 89,
            // unused
            ColumnId_Swims = 91,
            ColumnId_Flies = 92,
            ColumnId_Walks = 93,
            ColumnId_Essential = 94,
            ColumnId_BloodType = 95,
            // unused
            ColumnId_OpenSound = 97,
            ColumnId_CloseSound = 98,
            ColumnId_Duration = 99,
            ColumnId_Radius = 100,
            ColumnId_Colour = 101,
            ColumnId_Sound = 102,
            ColumnId_Dynamic = 103,
            ColumnId_Portable = 104,
            ColumnId_NegativeLight = 105,
            ColumnId_EmitterType = 106,
            // unused (3x)
            ColumnId_Fire = 110,
            ColumnId_OffByDefault = 111,
            ColumnId_IsKey = 112,
            ColumnId_Race = 113,
            ColumnId_Class = 114,
            Columnid_Hair = 115,
            ColumnId_Head = 116,
            ColumnId_Female = 117,
            ColumnId_WeaponType = 118,
            ColumnId_WeaponSpeed = 119,
            ColumnId_WeaponReach = 120,
            ColumnId_MinChop = 121,
            ColumnId_MaxChip = 122,
            Columnid_MinSlash = 123,
            ColumnId_MaxSlash = 124,
            ColumnId_MinThrust = 125,
            ColumnId_MaxThrust = 126,
            ColumnId_Magical = 127,
            ColumnId_Silver = 128,
            ColumnId_Filter = 129,
            ColumnId_PositionXPos = 130,
            ColumnId_PositionYPos = 131,
            ColumnId_PositionZPos = 132,
            ColumnId_PositionXRot = 133,
            ColumnId_PositionYRot = 134,
            ColumnId_PositionZRot = 135,
            ColumnId_DoorPositionXPos = 136,
            ColumnId_DoorPositionYPos = 137,
            ColumnId_DoorPositionZPos = 138,
            ColumnId_DoorPositionXRot = 139,
            ColumnId_DoorPositionYRot = 140,
            ColumnId_DoorPositionZRot = 141,
            ColumnId_DialogueType = 142,
            ColumnId_QuestIndex = 143,
            ColumnId_QuestStatusType = 144,
            ColumnId_QuestDescription = 145,
            ColumnId_Topic = 146,
            ColumnId_Journal = 147,
            ColumnId_Actor = 148,
            ColumnId_PcFaction = 149,
            ColumnId_Response = 150,
            ColumnId_Disposition = 151,
            ColumnId_Rank = 152,
            ColumnId_Gender = 153,
            ColumnId_PcRank = 154,
            ColumnId_ReferenceableId = 155,
            ColumnId_ContainerContent = 156,
            ColumnId_ItemCount = 157,
            ColumnId_InventoryItemId = 158,
            ColumnId_CombatState = 159,
            ColumnId_MagicState = 160,
            ColumnId_StealthState = 161,
            ColumnId_EnchantmentType = 162,
            ColumnId_Vampire = 163,
            ColumnId_BodyPartType = 164,
            ColumnId_MeshType = 165,
            ColumnId_ActorInventory = 166,
            ColumnId_SpellList = 167,
            ColumnId_SpellId = 168,
            ColumnId_NpcDestinations = 169,
            ColumnId_DestinationCell = 170,
            ColumnId_PosX = 171, // these are float
            ColumnId_PosY = 172, // these are float
            ColumnId_PosZ = 173, // these are float
            ColumnId_RotX = 174,
            ColumnId_RotY = 175,
            ColumnId_RotZ = 176,
            // unused
            ColumnId_OwnerGlobal = 178,
            ColumnId_DefaultProfile = 179,
            ColumnId_BypassNewGame = 180,
            ColumnId_GlobalProfile = 181,
            ColumnId_RefNumCounter = 182,
            ColumnId_RefNum = 183,
            ColumnId_Creature = 184,
            ColumnId_SoundGeneratorType = 185,
            ColumnId_AllowSpellmaking = 186,
            ColumnId_AllowEnchanting = 187,
            ColumnId_BaseCost = 188,
            ColumnId_School = 189,
            ColumnId_Particle = 190,
            ColumnId_CastingObject = 191,
            ColumnId_HitObject = 192,
            ColumnId_AreaObject = 193,
            ColumnId_BoltObject = 194,
            ColumnId_CastingSound = 195,
            ColumnId_HitSound = 196,
            ColumnId_AreaSound = 197,
            ColumnId_BoltSound = 198,

            ColumnId_PathgridPoints = 199,
            ColumnId_PathgridIndex = 200,
            ColumnId_PathgridPosX = 201, // these are int
            ColumnId_PathgridPosY = 202, // these are int
            ColumnId_PathgridPosZ = 203, // these are int
            ColumnId_PathgridEdges = 204,
            ColumnId_PathgridEdgeIndex = 205,
            ColumnId_PathgridEdge0 = 206,
            ColumnId_PathgridEdge1 = 207,

            ColumnId_RegionSounds = 208,
            ColumnId_SoundName = 209,
            ColumnId_SoundChance = 210,

            ColumnId_FactionReactions = 211,
            ColumnId_FactionReaction = 213,

            ColumnId_EffectList = 214,
            ColumnId_EffectId = 215,
            ColumnId_EffectRange = 217,
            ColumnId_EffectArea = 218,

            ColumnId_AiPackageList = 219,
            ColumnId_AiPackageType = 220,
            ColumnId_AiWanderDist = 221,
            ColumnId_AiDuration = 222,
            ColumnId_AiWanderToD = 223,
            // unused
            ColumnId_AiWanderRepeat = 225,
            ColumnId_AiActivateName = 226,
            // use ColumnId_PosX, etc for AI destinations
            ColumnId_AiTargetId = 227,
            ColumnId_AiTargetCell = 228,

            ColumnId_PartRefList = 229,
            ColumnId_PartRefType = 230,
            ColumnId_PartRefMale = 231,
            ColumnId_PartRefFemale = 232,

            ColumnId_LevelledList = 233,
            ColumnId_LevelledItemId = 234,
            ColumnId_LevelledItemLevel = 235,
            ColumnId_LevelledItemType = 236,
            ColumnId_LevelledItemTypeEach = 237,
            ColumnId_LevelledItemChanceNone = 238,

            ColumnId_PowerList = 239,
            ColumnId_Skill = 240,

            ColumnId_InfoList = 241,
            ColumnId_InfoCondition = 242,
            ColumnId_InfoCondFunc = 243,
            ColumnId_InfoCondVar = 244,
            ColumnId_InfoCondComp = 245,
            ColumnId_InfoCondValue = 246,

            ColumnId_OriginalCell = 247,

            ColumnId_NpcAttributes = 248,
            ColumnId_NpcSkills = 249,
            ColumnId_UChar = 250,
            ColumnId_NpcMisc = 251,
            ColumnId_Level = 252,
            // unused
            ColumnId_Mana = 255,
            ColumnId_Fatigue = 256,
            ColumnId_NpcDisposition = 257,
            ColumnId_NpcReputation = 258,
            ColumnId_NpcRank = 259,
            ColumnId_Gold = 260,
            ColumnId_NpcPersistence = 261,

            ColumnId_RaceAttributes = 262,
            ColumnId_Male = 263,
            // unused
            ColumnId_RaceSkillBonus = 265,
            // unused
            ColumnId_RaceBonus = 267,

            ColumnId_Interior = 268,
            ColumnId_Ambient = 269,
            ColumnId_Sunlight = 270,
            ColumnId_Fog = 271,
            ColumnId_FogDensity = 272,
            ColumnId_WaterLevel = 273,
            ColumnId_MapColor = 274,

            ColumnId_FileFormat = 275,
            ColumnId_FileDescription = 276,
            ColumnId_Author = 277,

            ColumnId_MinMagnitude = 278,
            ColumnId_MaxMagnitude = 279,

            ColumnId_CreatureAttributes = 280,
            ColumnId_AttributeValue = 281,
            ColumnId_CreatureAttack = 282,
            ColumnId_MinAttack = 283,
            ColumnId_MaxAttack = 284,
            ColumnId_CreatureMisc = 285,

            ColumnId_Idle1 = 286,
            ColumnId_Idle2 = 287,
            ColumnId_Idle3 = 288,
            ColumnId_Idle4 = 289,
            ColumnId_Idle5 = 290,
            ColumnId_Idle6 = 291,
            ColumnId_Idle7 = 292,
            ColumnId_Idle8 = 293,

            ColumnId_RegionWeather = 294,
            ColumnId_WeatherName = 295,
            ColumnId_WeatherChance = 296,

            ColumnId_Text = 297,

            ColumnId_TextureNickname = 298,
            ColumnId_PluginIndex = 299,
            ColumnId_TextureIndex = 300,
            ColumnId_LandMapLodIndex = 301,
            ColumnId_LandNormalsIndex = 302,
            ColumnId_LandHeightsIndex = 303,
            ColumnId_LandColoursIndex = 304,
            ColumnId_LandTexturesIndex = 305,

            ColumnId_RankName = 306,
            ColumnId_FactionRanks = 307,
            ColumnId_FactionPrimSkill = 308,
            ColumnId_FactionFavSkill = 309,
            ColumnId_FactionRep = 310,
            ColumnId_FactionAttrib1 = 311,
            ColumnId_FactionAttrib2 = 312,

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
            ColumnId_Skill6 = 0x50005,
            ColumnId_Skill7 = 0x50006
        };

        std::string getName (ColumnId column);

        int getId (const std::string& name);
        ///< Will return -1 for an invalid name.

        bool hasEnums (ColumnId column);

        std::vector<std::pair<int,std::string>> getEnums (ColumnId column);
        ///< Returns an empty vector, if \a column isn't an enum type column.
    }
}

#endif
