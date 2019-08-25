/*
  Copyright (C) 2016, 2018, 2019 cc9cii

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

#include <vector>

#include "common.hpp"

namespace ESM4
{
    class Reader;
    class Writer;

    struct Npc
    {
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
            std::uint8_t custom; // ?
        };

        struct Data
        {
            SkillValues   skills;
            std::uint32_t health;
            AttributeValues attribs;
        };

        struct ACBS // FIXME: this is ActorBaseConfig but for TES5
        {
            // 0x00000001 - Female
            // 0x00000002 - Essential
            // 0x00000004 - Is CharGen Face Preset
            // 0x00000008 - Respawn
            // 0x00000010 - Auto calc stats
            // 0x00000020 - Unique
            // 0x00000040 - Doesn't affect stealth meter
            // 0x00000080 - PC Level Mult
            // 0x00000100 - Audio template? (not displayed in CK)
            // 0x00000800 - Protected
            // 0x00004000 - Summonable
            // 0x00010000 - Doesn't Bleed
            // 0x00040000 - owned/follow? (Horses, Atronachs, NOT Shadowmere; not displayed in CK)
            // 0x00080000 - Opposite Gender Anims
            // 0x00100000 - Simple Actor
            // 0x00200000 - looped script? AAvenicci, Arcadia, Silvia, Afflicted, TortureVictims
            // 0x10000000 - looped audio? AAvenicci, Arcadia, Silvia, DA02 Cultists, Afflicted, TortureVictims
            // 0x20000000 - Ghost/non-interactable (Ghosts, Nocturnal)
            // 0x80000000 - Invulnerable
            std::uint32_t flags;
            std::uint16_t magickaOffset;
            std::uint16_t staminaOffset;
            std::uint16_t level; //(if PC Level Mult false) or [PC Level Multiplier]x1000 (if PC Level Mult true)
            std::uint16_t calcMinlevel;
            std::uint16_t calcMaxlevel;
            std::uint16_t speedMultiplier;
            std::uint16_t dispositionBase;
            // 0x0001 - Use traits (Destructible Object; Traits tab, including race, gender, height, weight,
            //          voice type, death item; Sounds tab; Animation tab; Character Gen tabs)
            // 0x0002 - Use stats (Stats tab, including level, autocalc, skills, health/magicka/stamina,
            //          speed, bleedout, class)
            // 0x0004 - Use factions (both factions and assigned crime faction)
            // 0x0008 - Use spelllist (both spells and perks)
            // 0x0010 - Use AI Data (AI Data tab, including aggression/confidence/morality, combat style and
            //          gift filter)
            // 0x0020 - Use AI Packages (only the basic Packages listed on the AI Packages tab;
            //          rest of tab controlled by Def Pack List)
            // 0x0040 - (unused?)
            // 0x0080 - Use Base Data (including name and short name, and flags for Essential, Protected,
            //          Respawn, Summonable, Simple Actor, and Doesn't affect stealth meter)
            // 0x0100 - Use inventory (Inventory tab, including all outfits and geared-up item
            //          -- but not death item)
            // 0x0200 - Use script
            // 0x0400 - Use Def Pack List (the dropdown-selected package lists on the AI Packages tab)
            // 0x0800 - Use Attack Data (Attack Data tab, including override from behavior graph race,
            //          events, and data)
            // 0x1000 - Use keywords
            std::uint16_t templateDataFlags; //(controls which parts of NPC Record are overwritten by the template)
            std::uint16_t healthOffset;
            std::uint16_t bleedoutOverride;
        };
#pragma pack(pop)

        FormId mFormId;       // from the header
        std::uint32_t mFlags; // from the header, see enum type RecordFlag for details

        std::string mEditorId;
        std::string mFullName;
        std::string mModel;

        FormId mRace;
        FormId mClass;
        FormId mHair;
        FormId mEyes;

        float mHairLength;
        HairColour mHairColour;

        FormId mDeathItem;
        std::vector<FormId> mSpell;
        FormId mScript;

        AIData mAIData;
        std::vector<FormId> mAIPackages;
        ActorBaseConfig mBaseConfig; // TES4
        ACBS mActorBaseConfig;       // TES5
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

        FormId mBaseTemplate; // TES5 only
        FormId mWornArmor;    // TES5 only

        Npc();
        virtual ~Npc();

        virtual void load(ESM4::Reader& reader);
        //virtual void save(ESM4::Writer& writer) const;

        //void blank();
    };
}

#endif // ESM4_NPC__H
