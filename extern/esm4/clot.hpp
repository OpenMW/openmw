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
#ifndef ESM4_CLOT_H
#define ESM4_CLOT_H

#include <string>
#include <cstdint>

namespace ESM4
{
    class Reader;
    class Writer;
    typedef std::uint32_t FormId;

    struct Clothing
    {
        enum Flags
        {
            // Biped Object Flags
            Flag_Head        = 0x00000001,
            Flag_Hair        = 0x00000002,
            Flag_UpperBody   = 0x00000004,
            Flag_LowerBody   = 0x00000008,
            Flag_Hand        = 0x00000010,
            Flag_Foot        = 0x00000020,
            Flag_RightRing   = 0x00000040,
            Flag_LeftRing    = 0x00000080,
            Flag_Amulet      = 0x00000100,
            Flag_Weapon      = 0x00000200,
            Flag_BackWeapon  = 0x00000400,
            Flag_SideWeapon  = 0x00000800,
            Flag_Quiver      = 0x00001000,
            Flag_Shield      = 0x00002000,
            Flag_Torch       = 0x00004000,
            Flag_Tail        = 0x00008000,
            // General Flags
            Flag_HideRings   = 0x00010000,
            Flag_HideAmulet  = 0x00020000,
            Flag_NonPlayable = 0x00400000,
            Flag_Unknown     = 0xCD000000
        };

#pragma pack(push, 1)
        struct Data
        {
            std::uint32_t value;   // gold
            float         weight;
        };
#pragma pack(pop)

        FormId mFormId;       // from the header
        std::uint32_t mFlags; // from the header, see enum type RecordFlag for details

        std::string mEditorId;
        std::string mFullName;
        std::string mModel;
        std::string mIconMale;   // inventory
        std::string mIconFemale; // inventory

        float mBoundRadius;

        std::uint32_t mClothingFlags;
        FormId        mScript;
        std::uint16_t mEnchantmentPoints;
        FormId        mEnchantment;

        Data mData;

        Clothing();
        virtual ~Clothing();

        virtual void load(ESM4::Reader& reader);
        //virtual void save(ESM4::Writer& writer) const;

        //void blank();
    };
}

#endif // ESM4_CLOT_H
