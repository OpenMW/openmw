/*
  Copyright (C) 2016, 2018-2021 cc9cii

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.

  cc9cii cc9c@iinet.net.au

  Much of the information on the data structures are based on the information
  from Tes4Mod:Mod_File_Format and Tes5Mod:File_Formats but also refined by
  trial & error.  See http://en.uesp.net/wiki for details.

*/
#ifndef ESM4_NPC__H
#define ESM4_NPC__H

#include <cstdint>
#include <string>
#include <vector>

#include "actor.hpp"
#include "inventory.hpp"

namespace ESM4
{
    class Reader;
    class Writer;

    struct Npc
    {
        enum ACBS_TES4
        {
            TES4_Female         = 0x000001,
            TES4_Essential      = 0x000002,
            TES4_Respawn        = 0x000008,
            TES4_AutoCalcStats  = 0x000010,
            TES4_PCLevelOffset  = 0x000080,
            TES4_NoLowLevelProc = 0x000200,
            TES4_NoRumors       = 0x002000,
            TES4_Summonable     = 0x004000,
            TES4_NoPersuasion   = 0x008000, // different meaning to crea
            TES4_CanCorpseCheck = 0x100000  // opposite of crea
        };

        enum ACBS_FO3
        {
            FO3_Female         = 0x00000001,
            FO3_Essential      = 0x00000002,
            FO3_PresetFace     = 0x00000004, // Is CharGen Face Preset
            FO3_Respawn        = 0x00000008,
            FO3_AutoCalcStats  = 0x00000010,
            FO3_PCLevelMult    = 0x00000080,
            FO3_UseTemplate    = 0x00000100,
            FO3_NoLowLevelProc = 0x00000200,
            FO3_NoBloodSpray   = 0x00000800,
            FO3_NoBloodDecal   = 0x00001000,
            FO3_NoVATSMelee    = 0x00100000,
            FO3_AnyRace        = 0x00400000,
            FO3_AutoCalcServ   = 0x00800000,
            FO3_NoKnockdown    = 0x04000000,
            FO3_NotPushable    = 0x08000000,
            FO3_NoRotateHead   = 0x40000000
        };

        enum ACBS_TES5
        {
            TES5_Female         = 0x00000001,
            TES5_Essential      = 0x00000002,
            TES5_PresetFace     = 0x00000004, // Is CharGen Face Preset
            TES5_Respawn        = 0x00000008,
            TES5_AutoCalcStats  = 0x00000010,
            TES5_Unique         = 0x00000020,
            TES5_NoStealth      = 0x00000040, // Doesn't affect stealth meter
            TES5_PCLevelMult    = 0x00000080,
          //TES5_Unknown        = 0x00000100, // Audio template?
            TES5_Protected      = 0x00000800,
            TES5_Summonable     = 0x00004000,
            TES5_NoBleeding     = 0x00010000,
            TES5_Owned          = 0x00040000, // owned/follow? (Horses, Atronachs, NOT Shadowmere)
            TES5_GenderAnim     = 0x00080000, // Opposite Gender Anims
            TES5_SimpleActor    = 0x00100000,
            TES5_LoopedScript   = 0x00200000, // AAvenicci, Arcadia, Silvia, Afflicted, TortureVictims
            TES5_LoopedAudio    = 0x10000000, // AAvenicci, Arcadia, Silvia, DA02 Cultists, Afflicted, TortureVictims
            TES5_IsGhost        = 0x20000000, // Ghost/non-interactable (Ghosts, Nocturnal)
            TES5_Invulnerable   = 0x80000000
        };

        enum Template_Flags
        {
            TES5_UseTraits    = 0x0001, // Destructible Object; Traits tab, including race, gender, height, weight,
                                        // voice type, death item; Sounds tab; Animation tab; Character Gen tabs
            TES5_UseStats     = 0x0002, // Stats tab, including level, autocalc, skills, health/magicka/stamina,
                                        // speed, bleedout, class
            TES5_UseFactions  = 0x0004, // both factions and assigned crime faction
            TES5_UseSpellList = 0x0008, // both spells and perks
            TES5_UseAIData    = 0x0010, // AI Data tab, including aggression/confidence/morality, combat style and
                                        // gift filter
            TES5_UseAIPackage = 0x0020, // only the basic Packages listed on the AI Packages tab;
                                        // rest of tab controlled by Def Pack List
            TES5_UseBaseData  = 0x0080, // including name and short name, and flags for Essential, Protected,
                                        // Respawn, Summonable, Simple Actor, and Doesn't affect stealth meter
            TES5_UseInventory = 0x0100, // Inventory tab, including all outfits and geared-up item
                                        // -- but not death item
            TES5_UseScript    = 0x0200,
            TES5_UseDefined   = 0x0400, // Def Pack List (the dropdown-selected package lists on the AI Packages tab)
            TES5_UseAtkData   = 0x0800, // Attack Data tab, including override from behavior graph race,
                                        // events, and data)
            TES5_UseKeywords  = 0x1000
        };

#pragma pack(push, 1)
        struct SkillValues
        {
            std::uint8_t  armorer;
            std::uint8_t  athletics;
            std::uint8_t  blade;
            std::uint8_t  block;
            std::uint8_t  blunt;
            std::uint8_t  handToHand;
            std::uint8_t  heavyArmor;
            std::uint8_t  alchemy;
            std::uint8_t  alteration;
            std::uint8_t  conjuration;
            std::uint8_t  destruction;
            std::uint8_t  illusion;
            std::uint8_t  mysticism;
            std::uint8_t  restoration;
            std::uint8_t  acrobatics;
            std::uint8_t  lightArmor;
            std::uint8_t  marksman;
            std::uint8_t  mercantile;
            std::uint8_t  security;
            std::uint8_t  sneak;
            std::uint8_t  speechcraft;
        };

        struct HairColour
        {
            std::uint8_t red;
            std::uint8_t green;
            std::uint8_t blue;
            std::uint8_t custom; // alpha?
        };

        struct Data
        {
            SkillValues   skills;
            std::uint32_t health;
            AttributeValues attribs;
        };

#pragma pack(pop)

        FormId mFormId;       // from the header
        std::uint32_t mFlags; // from the header, see enum type RecordFlag for details

        bool mIsTES4;
        bool mIsFONV;

        std::string mEditorId;
        std::string mFullName;
        std::string mModel;  // skeleton model (can be a marker in FO3/FONV)

        FormId mRace;
        FormId mClass;
        FormId mHair; // not for TES5, see mHeadParts
        FormId mEyes;

        std::vector<FormId> mHeadParts; // FO3/FONV/TES5

        float mHairLength;
        HairColour mHairColour; // TES4/FO3/FONV
        FormId mHairColourId; // TES5

        FormId mDeathItem;
        std::vector<FormId> mSpell;
        FormId mScriptId;

        AIData mAIData;
        std::vector<FormId> mAIPackages; // seems to be in priority order, 0 = highest priority
        ActorBaseConfig mBaseConfig; // union
        ActorFaction mFaction;
        Data   mData;
        FormId mCombatStyle;
        FormId mSoundBase;
        FormId mSound;
        std::uint8_t mSoundChance;
        float mFootWeight;

        float mBoundRadius;
        std::vector<std::string> mKf; // filenames only, get directory path from mModel

        std::vector<InventoryItem> mInventory;

        FormId mBaseTemplate; // FO3/FONV/TES5
        FormId mWornArmor;    // TES5 only?

        FormId mDefaultOutfit; // TES5 OTFT
        FormId mSleepOutfit;   // TES5 OTFT
        FormId mDefaultPkg;

        std::vector<float> mSymShapeModeCoefficients;    // size 0 or 50
        std::vector<float> mAsymShapeModeCoefficients;   // size 0 or 30
        std::vector<float> mSymTextureModeCoefficients;  // size 0 or 50
        std::int16_t mFgRace;

        Npc();
        virtual ~Npc();

        virtual void load(ESM4::Reader& reader);
        //virtual void save(ESM4::Writer& writer) const;

        //void blank();
    };
}

#endif // ESM4_NPC__H
