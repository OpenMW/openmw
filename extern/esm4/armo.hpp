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
#ifndef ESM4_ARMO_H
#define ESM4_ARMO_H

#include <string>
#include <cstdint>

namespace ESM4
{
    class Reader;
    class Writer;
    typedef std::uint32_t FormId;

    struct Armor
    {
        enum ArmorFlags
        {
            TES4_Head        = 0x00000001,
            TES4_Hair        = 0x00000002,
            TES4_UpperBody   = 0x00000004,
            TES4_LowerBody   = 0x00000008,
            TES4_Hand        = 0x00000010,
            TES4_Foot        = 0x00000020,
            TES4_RightRing   = 0x00000040,
            TES4_LeftRing    = 0x00000080,
            TES4_Amulet      = 0x00000100,
            TES4_Weapon      = 0x00000200,
            TES4_BackWeapon  = 0x00000400,
            TES4_SideWeapon  = 0x00000800,
            TES4_Quiver      = 0x00001000,
            TES4_Shield      = 0x00002000,
            TES4_Torch       = 0x00004000,
            TES4_Tail        = 0x00008000,
            //
            FO3_Head         = 0x00000001,
            FO3_Hair         = 0x00000002,
            FO3_UpperBody    = 0x00000004,
            FO3_LeftHand     = 0x00000008,
            FO3_RightHand    = 0x00000010,
            FO3_Weapon       = 0x00000020,
            FO3_PipBoy       = 0x00000040,
            FO3_Backpack     = 0x00000080,
            FO3_Necklace     = 0x00000100,
            FO3_Headband     = 0x00000200,
            FO3_Hat          = 0x00000400,
            FO3_EyeGlasses   = 0x00000800,
            FO3_NoseRing     = 0x00001000,
            FO3_Earrings     = 0x00002000,
            FO3_Mask         = 0x00004000,
            FO3_Choker       = 0x00008000,
            FO3_MouthObject  = 0x00010000,
            FO3_BodyAddOn1   = 0x00020000,
            FO3_BodyAddOn2   = 0x00040000,
            FO3_BodyAddOn3   = 0x00080000
        };

        enum GeneralFlags
        {
            TYPE_TES4        = 0x1000,
            TYPE_FO3         = 0x2000,
            TYPE_TES5        = 0x3000,
            TYPE_FONV        = 0x4000,
            //
            TES4_HideRings   = 0x0001,
            TES4_HideAmulet  = 0x0002,
            TES4_NonPlayable = 0x0040,
            TES4_HeavyArmor  = 0x0080,
            //
            FO3_PowerArmor   = 0x0020,
            FO3_NonPlayable  = 0x0040,
            FO3_HeavyArmor   = 0x0080,
            //
            TES5_LightArmor  = 0x0000,
            TES5_HeavyArmor  = 0x0001,
            TES5_None        = 0x0002,
            TES5_ModVoice    = 0x0004, // note bit shift
            TES5_NonPlayable = 0x0040  // note bit shift
        };

#pragma pack(push, 1)
        struct Data
        {
            std::uint16_t armor; // only in TES4?
            std::uint32_t value;
            std::uint32_t health; // not in TES5?
            float         weight;
        };
#pragma pack(pop)

        FormId mFormId;       // from the header
        std::uint32_t mFlags; // from the header, see enum type RecordFlag for details

        std::string mEditorId;
        std::string mFullName;
        std::string mModel;
        std::string mText;
        std::string mIconMale;
        std::string mIconFemale;

        float mBoundRadius;

        std::uint32_t mArmorFlags;
        std::uint32_t mGeneralFlags;
        FormId        mScript;
        std::uint16_t mEnchantmentPoints;
        FormId        mEnchantment;

        FormId mARMA;
        Data mData;

        Armor();
        virtual ~Armor();

        virtual void load(ESM4::Reader& reader);
        //virtual void save(ESM4::Writer& writer) const;

        //void blank();
    };
}

#endif // ESM4_ARMO_H
