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
#ifndef ESM4_RACE
#define ESM4_RACE

#include <string>
#include <cstdint>
#include <map>
#include <vector>

#include "common.hpp" // AttribValues

namespace ESM4
{
    class Reader;
    class Writer;
    typedef std::uint32_t FormId;

    struct Race
    {
#pragma pack(push, 1)
        struct Data
        {
            std::uint8_t flags; // 0x01 = not playable, 0x02 = not male, 0x04 = not female, ?? = fixed
        };
#pragma pack(pop)

        enum SkillIndex
        {
            Skill_Armorer     = 0x0C,
            Skill_Athletics   = 0x0D,
            Skill_Blade       = 0x0E,
            Skill_Block       = 0x0F,
            Skill_Blunt       = 0x10,
            Skill_HandToHand  = 0x11,
            Skill_HeavyArmor  = 0x12,
            Skill_Alchemy     = 0x13,
            Skill_Alteration  = 0x14,
            Skill_Conjuration = 0x15,
            Skill_Destruction = 0x16,
            Skill_Illusion    = 0x17,
            Skill_Mysticism   = 0x18,
            Skill_Restoration = 0x19,
            Skill_Acrobatics  = 0x1A,
            Skill_LightArmor  = 0x1B,
            Skill_Marksman    = 0x1C,
            Skill_Mercantile  = 0x1D,
            Skill_Security    = 0x1E,
            Skill_Sneak       = 0x1F,
            Skill_Speechcraft = 0x20,
            Skill_None        = 0xFF,
            Skill_Unknown     = 0x00
        };

        FormId mFormId;       // from the header
        std::uint32_t mFlags; // from the header, see enum type RecordFlag for details

        std::string mEditorId;
        std::string mFullName;
        std::string mModel;
        std::string mMaleModel;
        std::string mFemaleModel;
        std::string mIcon; // inventory
        std::string mDesc;

        float mBoundRadius;

        AttributeValues mAttribMale;
        AttributeValues mAttribFemale;
        std::map<SkillIndex, std::uint8_t> mSkillBonus;
        float mHeightMale;
        float mHeightFemale;
        float mWeightMale;
        float mWeightFemale;
        std::uint32_t mRaceFlags; // 0x0001 = playable?
        std::uint32_t mNumKeywords;
        std::vector<FormId> mEyeChoices;
        std::vector<FormId> mHairChoices;

        std::map<FormId, std::int32_t> mDisposition; // race adjustments
        std::vector<FormId> mBonusSpells; // race ability/power
        std::vector<FormId> mVNAM; // don't know what these are; 1 or 2 RACE FormIds
        std::vector<FormId> mDecapitate; // male/female (HAIR FormId for TES4)

        Data mData;

        Race();
        virtual ~Race();

        virtual void load(ESM4::Reader& reader);
        //virtual void save(ESM4::Writer& writer) const;

        //void blank();
    };
}

#endif // ESM4_RACE
