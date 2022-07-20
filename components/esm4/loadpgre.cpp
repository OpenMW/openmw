/*
  Copyright (C) 2020 cc9cii

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

  Also see https://tes5edit.github.io/fopdoc/ for FO3/FONV specific details.

*/
#include "loadpgre.hpp"

#include <stdexcept>
#include <iostream> // FIXME: for debugging only

#include "reader.hpp"
//#include "writer.hpp"

void ESM4::PlacedGrenade::load(ESM4::Reader& reader)
{
    mFormId = reader.hdr().record.id;
    reader.adjustFormId(mFormId);
    mFlags  = reader.hdr().record.flags;

    while (reader.getSubRecordHeader())
    {
        const ESM4::SubRecordHeader& subHdr = reader.subRecordHeader();
        switch (subHdr.typeId)
        {
            case ESM4::SUB_EDID: reader.getZString(mEditorId);  break;
            case ESM4::SUB_NAME:
            case ESM4::SUB_XEZN:
            case ESM4::SUB_XRGD:
            case ESM4::SUB_XRGB:
            case ESM4::SUB_XPRD:
            case ESM4::SUB_XPPA:
            case ESM4::SUB_INAM:
            case ESM4::SUB_TNAM:
            case ESM4::SUB_XOWN:
            case ESM4::SUB_XRNK:
            case ESM4::SUB_XCNT:
            case ESM4::SUB_XRDS:
            case ESM4::SUB_XHLP:
            case ESM4::SUB_XPWR:
            case ESM4::SUB_XDCR:
            case ESM4::SUB_XLKR:
            case ESM4::SUB_XCLP:
            case ESM4::SUB_XAPD:
            case ESM4::SUB_XAPR:
            case ESM4::SUB_XATO:
            case ESM4::SUB_XESP:
            case ESM4::SUB_XEMI:
            case ESM4::SUB_XMBR:
            case ESM4::SUB_XIBS:
            case ESM4::SUB_XSCL:
            case ESM4::SUB_DATA:
            {
                //std::cout << "PGRE " << ESM::printName(subHdr.typeId) << " skipping..."
                          //<< subHdr.dataSize << std::endl;
                reader.skipSubRecordData();
                break;
            }
            default:
                std::cout << "PGRE " << ESM::printName(subHdr.typeId) << " skipping..."
                          << subHdr.dataSize << std::endl;
                reader.skipSubRecordData();
                //throw std::runtime_error("ESM4::PGRE::load - Unknown subrecord " + ESM::printName(subHdr.typeId));
        }
    }
}

//void ESM4::PlacedGrenade::save(ESM4::Writer& writer) const
//{
//}

//void ESM4::PlacedGrenade::blank()
//{
//}
