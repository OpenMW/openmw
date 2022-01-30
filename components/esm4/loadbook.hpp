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
#ifndef ESM4_BOOK_H
#define ESM4_BOOK_H

#include <cstdint>
#include <string>

#include "formid.hpp"

namespace ESM4
{
    class Reader;
    class Writer;

    struct Book
    {
        enum Flags
        {
            Flag_Scroll = 0x0001,
            Flag_NoTake = 0x0002
        };

        enum BookSkill // for TES4 only
        {
            BookSkill_None        = -1,
            BookSkill_Armorer     =  0,
            BookSkill_Athletics   =  1,
            BookSkill_Blade       =  2,
            BookSkill_Block       =  3,
            BookSkill_Blunt       =  4,
            BookSkill_HandToHand  =  5,
            BookSkill_HeavyArmor  =  6,
            BookSkill_Alchemy     =  7,
            BookSkill_Alteration  =  8,
            BookSkill_Conjuration =  9,
            BookSkill_Destruction = 10,
            BookSkill_Illusion    = 11,
            BookSkill_Mysticism   = 12,
            BookSkill_Restoration = 13,
            BookSkill_Acrobatics  = 14,
            BookSkill_LightArmor  = 15,
            BookSkill_Marksman    = 16,
            BookSkill_Mercantile  = 17,
            BookSkill_Security    = 18,
            BookSkill_Sneak       = 19,
            BookSkill_Speechcraft = 20
        };

        struct Data
        {
            std::uint8_t  flags;
            std::uint8_t  type;    // TES5 only
            std::uint32_t teaches; // TES5 only
            std::int8_t bookSkill; // not in TES5
            std::uint32_t value;
            float         weight;
        };

        FormId mFormId;       // from the header
        std::uint32_t mFlags; // from the header, see enum type RecordFlag for details

        std::string mEditorId;
        std::string mFullName;
        std::string mModel;

        float mBoundRadius;

        std::string mText;
        FormId      mScriptId;
        std::string mIcon;
        std::uint16_t mEnchantmentPoints;
        FormId      mEnchantment;

        Data mData;

        FormId mPickUpSound;
        FormId mDropSound;

        Book();
        virtual ~Book();

        virtual void load(ESM4::Reader& reader);
        //virtual void save(ESM4::Writer& writer) const;

        //void blank();
    };
}

#endif // ESM4_BOOK_H
