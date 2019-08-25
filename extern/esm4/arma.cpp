/*
  Copyright (C) 2019 cc9cii

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
#include "arma.hpp"

#include <stdexcept>
//#include <iostream> // FIXME: testing only

#include "reader.hpp"
//#include "writer.hpp"

ESM4::ArmorAddon::ArmorAddon() : mFormId(0), mFlags(0)
{
    mEditorId.clear();
}

ESM4::ArmorAddon::~ArmorAddon()
{
}

void ESM4::ArmorAddon::load(ESM4::Reader& reader)
{
    mFormId = reader.hdr().record.id;
    reader.adjustFormId(mFormId);
    mFlags  = reader.hdr().record.flags;

    while (reader.getSubRecordHeader())
    {
        const ESM4::SubRecordHeader& subHdr = reader.subRecordHeader();
        switch (subHdr.typeId)
        {
            case ESM4::SUB_EDID: reader.getZString(mEditorId); break;
            case ESM4::SUB_RNAM:
            case ESM4::SUB_BODT:
            case ESM4::SUB_DNAM:
            case ESM4::SUB_MO2S:
            case ESM4::SUB_MO2T:
            case ESM4::SUB_MO3S:
            case ESM4::SUB_MO3T:
            case ESM4::SUB_MO4S:
            case ESM4::SUB_MO4T:
            case ESM4::SUB_MO5T:
            case ESM4::SUB_MOD2:
            case ESM4::SUB_MOD3:
            case ESM4::SUB_MOD4:
            case ESM4::SUB_MOD5:
            case ESM4::SUB_MODL:
            case ESM4::SUB_NAM0:
            case ESM4::SUB_NAM1:
            case ESM4::SUB_NAM2:
            case ESM4::SUB_NAM3:
            case ESM4::SUB_SNDD:
            case ESM4::SUB_BMDT: // FO3
            case ESM4::SUB_DATA: // FO3
            case ESM4::SUB_ETYP: // FO3
            case ESM4::SUB_FULL: // FO3
            case ESM4::SUB_ICO2: // FO3
            case ESM4::SUB_ICON: // FO3
            case ESM4::SUB_MODD: // FO3
            case ESM4::SUB_MODS: // FO3
            case ESM4::SUB_MODT: // FO3
            case ESM4::SUB_MOSD: // FO3
            case ESM4::SUB_OBND: // FO3
            {
                //std::cout << "ARMA " << ESM4::printName(subHdr.typeId) << " skipping..." << std::endl;
                reader.skipSubRecordData();
                break;
            }
            default:
                throw std::runtime_error("ESM4::ARMA::load - Unknown subrecord " + ESM4::printName(subHdr.typeId));
        }
    }
}

//void ESM4::ArmorAddon::save(ESM4::Writer& writer) const
//{
//}

//void ESM4::ArmorAddon::blank()
//{
//}
