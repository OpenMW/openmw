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
#include "loadacre.hpp"

#include <stdexcept>
//#include <iostream>

#include "reader.hpp"
//#include "writer.hpp"

void ESM4::ActorCreature::load(ESM4::Reader& reader)
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
            case ESM4::SUB_NAME: reader.getFormId(mBaseObj);   break;
            case ESM4::SUB_DATA: reader.get(mPlacement);       break;
            case ESM4::SUB_XSCL: reader.get(mScale);           break;
            case ESM4::SUB_XESP:
            {
                reader.get(mEsp);
                reader.adjustFormId(mEsp.parent);
                break;
            }
            case ESM4::SUB_XOWN: reader.getFormId(mOwner);     break;
            case ESM4::SUB_XGLB: reader.get(mGlobal);          break; // FIXME: formId?
            case ESM4::SUB_XRNK: reader.get(mFactionRank);     break;
            case ESM4::SUB_XRGD: // ragdoll
            case ESM4::SUB_XRGB: // ragdoll biped
            {
                // seems to occur only for dead bodies, e.g. DeadMuffy, DeadDogVicious
                //std::cout << "ACRE " << ESM::printName(subHdr.typeId) << " skipping..." << std::endl;
                reader.skipSubRecordData();
                break;
            }
            case ESM4::SUB_XLKR: // FO3
            case ESM4::SUB_XLCM: // FO3
            case ESM4::SUB_XEZN: // FO3
            case ESM4::SUB_XMRC: // FO3
            case ESM4::SUB_XAPD: // FO3
            case ESM4::SUB_XAPR: // FO3
            case ESM4::SUB_XRDS: // FO3
            case ESM4::SUB_XPRD: // FO3
            case ESM4::SUB_XATO: // FONV
            {
                //std::cout << "ACRE " << ESM::printName(subHdr.typeId) << " skipping..." << std::endl;
                reader.skipSubRecordData();
                break;
            }
            default:
                throw std::runtime_error("ESM4::ACRE::load - Unknown subrecord " + ESM::printName(subHdr.typeId));
        }
    }
}

//void ESM4::ActorCreature::save(ESM4::Writer& writer) const
//{
//}

//void ESM4::ActorCreature::blank()
//{
//}
