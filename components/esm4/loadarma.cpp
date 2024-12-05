/*
  Copyright (C) 2019, 2020 cc9cii

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
#include "loadarma.hpp"

#include <stdexcept>

#include "reader.hpp"
//#include "writer.hpp"

void ESM4::ArmorAddon::load(ESM4::Reader& reader)
{
    mId = reader.getFormIdFromHeader();
    mFlags = reader.hdr().record.flags;

    std::uint32_t esmVer = reader.esmVersion();

    while (reader.getSubRecordHeader())
    {
        const ESM4::SubRecordHeader& subHdr = reader.subRecordHeader();
        switch (subHdr.typeId)
        {
            case ESM::fourCC("EDID"):
                reader.getZString(mEditorId);
                break;
            case ESM::fourCC("MOD2"):
                reader.getZString(mModelMale);
                break;
            case ESM::fourCC("MOD3"):
                reader.getZString(mModelFemale);
                break;
            case ESM::fourCC("MOD4"):
            case ESM::fourCC("MOD5"):
            {
                std::string model;
                reader.getZString(model);
                break;
            }
            case ESM::fourCC("NAM0"):
                reader.getFormId(mTextureMale);
                break;
            case ESM::fourCC("NAM1"):
                reader.getFormId(mTextureFemale);
                break;
            case ESM::fourCC("RNAM"):
                reader.getFormId(mRacePrimary);
                break;
            case ESM::fourCC("MODL"):
                if ((esmVer == ESM::VER_094 || esmVer == ESM::VER_170) && subHdr.dataSize == 4) // TES5
                    reader.getFormId(mRaces.emplace_back());
                else
                    reader.skipSubRecordData(); // FIXME: this should be mModelMale for FO3/FONV

                break;
            case ESM::fourCC("BODT"): // body template
                reader.get(mBodyTemplate.bodyPart);
                reader.get(mBodyTemplate.flags);
                reader.get(mBodyTemplate.unknown1); // probably padding
                reader.get(mBodyTemplate.unknown2); // probably padding
                reader.get(mBodyTemplate.unknown3); // probably padding
                reader.get(mBodyTemplate.type);
                break;
            case ESM::fourCC("BOD2"): // TES5+
                reader.get(mBodyTemplate.bodyPart);
                mBodyTemplate.flags = 0;
                mBodyTemplate.unknown1 = 0; // probably padding
                mBodyTemplate.unknown2 = 0; // probably padding
                mBodyTemplate.unknown3 = 0; // probably padding
                mBodyTemplate.type = 0;
                if (subHdr.dataSize == 8)
                    reader.get(mBodyTemplate.type);

                break;
            case ESM::fourCC("DNAM"):
                if (subHdr.dataSize == 12)
                {
                    std::uint16_t unknownInt16;
                    std::uint8_t unknownInt8;
                    reader.get(mMalePriority);
                    reader.get(mFemalePriority);
                    reader.get(mWeightSliderMale);
                    reader.get(mWeightSliderFemale);
                    reader.get(unknownInt16);
                    reader.get(mDetectionSoundValue);
                    reader.get(unknownInt8);
                    reader.get(mWeaponAdjust);
                }
                else
                    reader.skipSubRecordData();
                break;
            case ESM::fourCC("MO2T"): // FIXME: should group with MOD2
            case ESM::fourCC("MO2S"): // FIXME: should group with MOD2
            case ESM::fourCC("MO2C"): // FIXME: should group with MOD2
            case ESM::fourCC("MO2F"): // FIXME: should group with MOD2
            case ESM::fourCC("MO3T"): // FIXME: should group with MOD3
            case ESM::fourCC("MO3S"): // FIXME: should group with MOD3
            case ESM::fourCC("MO3C"): // FIXME: should group with MOD3
            case ESM::fourCC("MO3F"): // FIXME: should group with MOD3
            case ESM::fourCC("MOSD"): // FO3 // FIXME: should group with MOD3
            case ESM::fourCC("MO4T"): // FIXME: should group with MOD4
            case ESM::fourCC("MO4S"): // FIXME: should group with MOD4
            case ESM::fourCC("MO4C"): // FIXME: should group with MOD4
            case ESM::fourCC("MO4F"): // FIXME: should group with MOD4
            case ESM::fourCC("MO5T"):
            case ESM::fourCC("MO5S"):
            case ESM::fourCC("MO5C"):
            case ESM::fourCC("MO5F"):
            case ESM::fourCC("NAM2"): // txst formid male
            case ESM::fourCC("NAM3"): // txst formid female
            case ESM::fourCC("SNDD"): // footset sound formid
            case ESM::fourCC("BMDT"): // FO3
            case ESM::fourCC("DATA"): // FO3
            case ESM::fourCC("ETYP"): // FO3
            case ESM::fourCC("FULL"): // FO3
            case ESM::fourCC("ICO2"): // FO3 // female
            case ESM::fourCC("ICON"): // FO3 // male
            case ESM::fourCC("MODT"): // FO3 // FIXME: should group with MODL
            case ESM::fourCC("MODS"): // FO3 // FIXME: should group with MODL
            case ESM::fourCC("MODD"): // FO3 // FIXME: should group with MODL
            case ESM::fourCC("OBND"): // FO3
            case ESM::fourCC("BSMB"): // FO4
            case ESM::fourCC("BSMP"): // FO4
            case ESM::fourCC("BSMS"): // FO4
            case ESM::fourCC("ONAM"): // FO4
                reader.skipSubRecordData();
                break;
            default:
                throw std::runtime_error("ESM4::ARMA::load - Unknown subrecord " + ESM::printName(subHdr.typeId));
        }
    }
}

// void ESM4::ArmorAddon::save(ESM4::Writer& writer) const
//{
// }

// void ESM4::ArmorAddon::blank()
//{
// }
