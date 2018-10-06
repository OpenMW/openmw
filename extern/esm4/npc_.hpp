/*
  Copyright (C) 2016, 2018 cc9cii

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

        struct Data
        {
            SkillValues   skills;
            std::uint32_t health;
            AttributeValues attribs;
        };

        FormId mFormId;       // from the header
        std::uint32_t mFlags; // from the header, see enum type RecordFlag for details

        std::string mEditorId;
        std::string mFullName;
        std::string mModel;

        FormId mRace;
        FormId mClass;
        FormId mHair;
        FormId mEyes;

        FormId mDeathItem;
        std::vector<FormId> mSpell;
        FormId mScript;

        AIData mAIData;
        std::vector<FormId> mAIPackages;
        ActorBaseConfig mBaseConfig;
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

        Npc();
        virtual ~Npc();

        virtual void load(ESM4::Reader& reader);
        //virtual void save(ESM4::Writer& writer) const;

        //void blank();
    };
}

#endif // ESM4_NPC__H
