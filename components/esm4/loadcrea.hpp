/*
  Copyright (C) 2016, 2018, 2020 cc9cii

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
#ifndef ESM4_CREA_H
#define ESM4_CREA_H

#include <cstdint>
#include <string>
#include <vector>

#include "actor.hpp"
#include "inventory.hpp"

namespace ESM4
{
    class Reader;
    class Writer;

    struct Creature
    {
        enum ACBS_TES4
        {
            TES4_Essential      = 0x000002,
            TES4_WeapAndShield  = 0x000004,
            TES4_Respawn        = 0x000008,
            TES4_PCLevelOffset  = 0x000080,
            TES4_NoLowLevelProc = 0x000200,
            TES4_NoHead         = 0x008000, // different meaning to npc_
            TES4_NoRightArm     = 0x010000,
            TES4_NoLeftArm      = 0x020000,
            TES4_NoCombatWater  = 0x040000,
            TES4_NoShadow       = 0x080000,
            TES4_NoCorpseCheck  = 0x100000  // opposite of npc_
        };

        enum ACBS_FO3
        {
            FO3_Biped          = 0x00000001,
            FO3_Essential      = 0x00000002,
            FO3_Weap_Shield    = 0x00000004,
            FO3_Respawn        = 0x00000008,
            FO3_CanSwim        = 0x00000010,
            FO3_CanFly         = 0x00000020,
            FO3_CanWalk        = 0x00000040,
            FO3_PCLevelMult    = 0x00000080,
            FO3_NoLowLevelProc = 0x00000200,
            FO3_NoBloodSpray   = 0x00000800,
            FO3_NoBloodDecal   = 0x00001000,
            FO3_NoHead         = 0x00008000,
            FO3_NoRightArm     = 0x00010000,
            FO3_NoLeftArm      = 0x00020000,
            FO3_NoWaterCombat  = 0x00040000,
            FO3_NoShadow       = 0x00080000,
            FO3_NoVATSMelee    = 0x00100000,
            FO3_AllowPCDialog  = 0x00200000,
            FO3_NoOpenDoors    = 0x00400000,
            FO3_Immobile       = 0x00800000,
            FO3_TiltFrontBack  = 0x01000000,
            FO3_TiltLeftRight  = 0x02000000,
            FO3_NoKnockdown    = 0x04000000,
            FO3_NotPushable    = 0x08000000,
            FO3_AllowPickpoket = 0x10000000,
            FO3_IsGhost        = 0x20000000,
            FO3_NoRotateHead   = 0x40000000,
            FO3_Invulnerable   = 0x80000000
        };

#pragma pack(push, 1)
        struct Data
        {
            std::uint8_t  unknown;
            std::uint8_t  combat;
            std::uint8_t  magic;
            std::uint8_t  stealth;
            std::uint16_t soul;
            std::uint16_t health;
            std::uint16_t unknown2;
            std::uint16_t damage;
            AttributeValues attribs;
        };
#pragma pack(pop)

        FormId mFormId;       // from the header
        std::uint32_t mFlags; // from the header, see enum type RecordFlag for details

        std::string mEditorId;
        std::string mFullName;
        std::string mModel;

        FormId mDeathItem;
        std::vector<FormId> mSpell;
        FormId mScriptId;

        AIData mAIData;
        std::vector<FormId> mAIPackages;
        ActorBaseConfig mBaseConfig;
        ActorFaction mFaction;
        Data   mData;
        FormId mCombatStyle;
        FormId mSoundBase;
        FormId mSound;
        std::uint8_t mSoundChance;
        float mBaseScale;
        float mTurningSpeed;
        float mFootWeight;
        std::string mBloodSpray;
        std::string mBloodDecal;

        float mBoundRadius;
        std::vector<std::string> mNif; // NIF filenames, get directory from mModel
        std::vector<std::string> mKf;

        std::vector<InventoryItem> mInventory;

        FormId mBaseTemplate;           // FO3/FONV
        std::vector<FormId> mBodyParts; // FO3/FONV

        Creature();
        virtual ~Creature();

        virtual void load(ESM4::Reader& reader);
        //virtual void save(ESM4::Writer& writer) const;

        //void blank();
    };
}

#endif // ESM4_CREA_H
