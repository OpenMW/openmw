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

#include "reader.hpp"
//#include "writer.hpp"

void ESM4::PlacedGrenade::load(ESM4::Reader& reader)
{
    mId = reader.getFormIdFromHeader();
    mFlags = reader.hdr().record.flags;

    while (reader.getSubRecordHeader())
    {
        const ESM4::SubRecordHeader& subHdr = reader.subRecordHeader();
        switch (subHdr.typeId)
        {
            case ESM::fourCC("EDID"):
                reader.getZString(mEditorId);
                break;
            case ESM::fourCC("NAME"):
            case ESM::fourCC("XEZN"):
            case ESM::fourCC("XRGD"):
            case ESM::fourCC("XRGB"):
            case ESM::fourCC("XPRD"):
            case ESM::fourCC("XPPA"):
            case ESM::fourCC("INAM"):
            case ESM::fourCC("TNAM"):
            case ESM::fourCC("XOWN"):
            case ESM::fourCC("XRNK"):
            case ESM::fourCC("XCNT"):
            case ESM::fourCC("XRDS"):
            case ESM::fourCC("XHLP"):
            case ESM::fourCC("XPWR"):
            case ESM::fourCC("XDCR"):
            case ESM::fourCC("XLKR"):
            case ESM::fourCC("XLKT"): // FO4
            case ESM::fourCC("XCLP"):
            case ESM::fourCC("XAPD"):
            case ESM::fourCC("XAPR"):
            case ESM::fourCC("XATO"):
            case ESM::fourCC("XESP"):
            case ESM::fourCC("XEMI"):
            case ESM::fourCC("XMBR"):
            case ESM::fourCC("XIBS"):
            case ESM::fourCC("XSCL"):
            case ESM::fourCC("DATA"):
            case ESM::fourCC("VMAD"):
            case ESM::fourCC("MNAM"): // FO4
            case ESM::fourCC("XAMC"): // FO4
            case ESM::fourCC("XASP"): // FO4
            case ESM::fourCC("XATP"): // FO4
            case ESM::fourCC("XCVR"): // FO4
            case ESM::fourCC("XFVC"): // FO4
            case ESM::fourCC("XHTW"): // FO4
            case ESM::fourCC("XIS2"): // FO4
            case ESM::fourCC("XLOD"): // FO4
            case ESM::fourCC("XLRL"): // FO4
            case ESM::fourCC("XLRT"): // FO4
            case ESM::fourCC("XLYR"): // FO4
            case ESM::fourCC("XMSP"): // FO4
            case ESM::fourCC("XRFG"): // FO4
                reader.skipSubRecordData();
                break;
            default:
                throw std::runtime_error("ESM4::PGRE::load - Unknown subrecord " + ESM::printName(subHdr.typeId));
        }
    }
}

// void ESM4::PlacedGrenade::save(ESM4::Writer& writer) const
//{
// }

// void ESM4::PlacedGrenade::blank()
//{
// }
