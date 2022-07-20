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
//#include <iostream> // FIXME: testing only

#include "reader.hpp"
//#include "writer.hpp"

void ESM4::ArmorAddon::load(ESM4::Reader& reader)
{
    mFormId = reader.hdr().record.id;
    reader.adjustFormId(mFormId);
    mFlags  = reader.hdr().record.flags;

    std::uint32_t esmVer = reader.esmVersion();

    while (reader.getSubRecordHeader())
    {
        const ESM4::SubRecordHeader& subHdr = reader.subRecordHeader();
        switch (subHdr.typeId)
        {
            case ESM4::SUB_EDID: reader.getZString(mEditorId); break;
            case ESM4::SUB_MOD2: reader.getZString(mModelMale); break;
            case ESM4::SUB_MOD3: reader.getZString(mModelFemale); break;
            case ESM4::SUB_MOD4:
            case ESM4::SUB_MOD5:
            {
                std::string model;
                reader.getZString(model);

                //std::cout << mEditorId << " " << ESM::printName(subHdr.typeId) << " " << model << std::endl;

                break;
            }
            case ESM4::SUB_NAM0: reader.getFormId(mTextureMale); break;
            case ESM4::SUB_NAM1: reader.getFormId(mTextureFemale); break;
            case ESM4::SUB_RNAM: reader.getFormId(mRacePrimary); break;
            case ESM4::SUB_MODL:
            {
                if ((esmVer == ESM::VER_094 || esmVer == ESM::VER_170) && subHdr.dataSize == 4) // TES5
                {
                    FormId formId;
                    reader.getFormId(formId);
                    mRaces.push_back(formId);
                }
                else
                    reader.skipSubRecordData(); // FIXME: this should be mModelMale for FO3/FONV

                break;
            }
            case ESM4::SUB_BODT: // body template
            {
                reader.get(mBodyTemplate.bodyPart);
                reader.get(mBodyTemplate.flags);
                reader.get(mBodyTemplate.unknown1); // probably padding
                reader.get(mBodyTemplate.unknown2); // probably padding
                reader.get(mBodyTemplate.unknown3); // probably padding
                reader.get(mBodyTemplate.type);

                break;
            }
            case ESM4::SUB_BOD2: // TES5
            {
                reader.get(mBodyTemplate.bodyPart);
                mBodyTemplate.flags = 0;
                mBodyTemplate.unknown1 = 0; // probably padding
                mBodyTemplate.unknown2 = 0; // probably padding
                mBodyTemplate.unknown3 = 0; // probably padding
                reader.get(mBodyTemplate.type);

                break;
            }
            case ESM4::SUB_DNAM:
            case ESM4::SUB_MO2T: // FIXME: should group with MOD2
            case ESM4::SUB_MO2S: // FIXME: should group with MOD2
            case ESM4::SUB_MO3T: // FIXME: should group with MOD3
            case ESM4::SUB_MO3S: // FIXME: should group with MOD3
            case ESM4::SUB_MOSD: // FO3 // FIXME: should group with MOD3
            case ESM4::SUB_MO4T: // FIXME: should group with MOD4
            case ESM4::SUB_MO4S: // FIXME: should group with MOD4
            case ESM4::SUB_MO5T:
            case ESM4::SUB_NAM2: // txst formid male
            case ESM4::SUB_NAM3: // txst formid female
            case ESM4::SUB_SNDD: // footset sound formid
            case ESM4::SUB_BMDT: // FO3
            case ESM4::SUB_DATA: // FO3
            case ESM4::SUB_ETYP: // FO3
            case ESM4::SUB_FULL: // FO3
            case ESM4::SUB_ICO2: // FO3 // female
            case ESM4::SUB_ICON: // FO3 // male
            case ESM4::SUB_MODT: // FO3 // FIXME: should group with MODL
            case ESM4::SUB_MODS: // FO3 // FIXME: should group with MODL
            case ESM4::SUB_MODD: // FO3 // FIXME: should group with MODL
            case ESM4::SUB_OBND: // FO3
            {
                //std::cout << "ARMA " << ESM::printName(subHdr.typeId) << " skipping..." << std::endl;
                reader.skipSubRecordData();
                break;
            }
            default:
                throw std::runtime_error("ESM4::ARMA::load - Unknown subrecord " + ESM::printName(subHdr.typeId));
        }
    }
}

//void ESM4::ArmorAddon::save(ESM4::Writer& writer) const
//{
//}

//void ESM4::ArmorAddon::blank()
//{
//}
