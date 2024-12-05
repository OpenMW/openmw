/*
  Copyright (C) 2016, 2018, 2020-2021 cc9cii

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
#include "loadachr.hpp"

#include <stdexcept>

#include "reader.hpp"
//#include "writer.hpp"

void ESM4::ActorCharacter::load(ESM4::Reader& reader)
{
    mId = reader.getFormIdFromHeader();
    mFlags = reader.hdr().record.flags;
    mParent = reader.currCell();

    while (reader.getSubRecordHeader())
    {
        const ESM4::SubRecordHeader& subHdr = reader.subRecordHeader();
        switch (subHdr.typeId)
        {
            case ESM::fourCC("EDID"):
                reader.getZString(mEditorId);
                break;
            case ESM::fourCC("FULL"):
                reader.getZString(mFullName);
                break;
            case ESM::fourCC("NAME"):
                reader.getFormId(mBaseObj);
                break;
            case ESM::fourCC("DATA"):
                reader.get(mPos);
                break;
            case ESM::fourCC("XSCL"):
                reader.get(mScale);
                break;
            case ESM::fourCC("XOWN"):
            {
                switch (subHdr.dataSize)
                {
                    case 4:
                        reader.getFormId(mOwner);
                        break;
                    case 12:
                    {
                        reader.getFormId(mOwner);
                        std::uint32_t dummy;
                        reader.get(dummy); // Unknown
                        reader.get(dummy); // No crime flag, FO4
                        break;
                    }
                    default:
                        reader.skipSubRecordData();
                        break;
                }
                break;
            }
            case ESM::fourCC("XESP"):
                reader.getFormId(mEsp.parent);
                reader.get(mEsp.flags);
                break;
            case ESM::fourCC("XCNT"):
            {
                reader.get(mCount);
                break;
            }
            case ESM::fourCC("XRGD"): // ragdoll
            case ESM::fourCC("XRGB"): // ragdoll biped
            case ESM::fourCC("XHRS"): // horse formId
            case ESM::fourCC("XMRC"): // merchant container formId
            // TES5
            case ESM::fourCC("XAPD"): // activation parent
            case ESM::fourCC("XAPR"): // active parent
            case ESM::fourCC("XEZN"): // encounter zone
            case ESM::fourCC("XHOR"):
            case ESM::fourCC("XLCM"): // levelled creature
            case ESM::fourCC("XLCN"): // location
            case ESM::fourCC("XLKR"): // location route?
            case ESM::fourCC("XLRT"): // location type
            //
            case ESM::fourCC("XPRD"):
            case ESM::fourCC("XPPA"):
            case ESM::fourCC("INAM"):
            case ESM::fourCC("PDTO"):
            //
            case ESM::fourCC("XIS2"):
            case ESM::fourCC("XPCI"): // formId
            case ESM::fourCC("XLOD"):
            case ESM::fourCC("VMAD"):
            case ESM::fourCC("XLRL"): // Unofficial Skyrim Patch
            case ESM::fourCC("XRDS"): // FO3
            case ESM::fourCC("XIBS"): // FO3
            case ESM::fourCC("SCHR"): // FO3
            case ESM::fourCC("TNAM"): // FO3
            case ESM::fourCC("XATO"): // FONV
            case ESM::fourCC("MNAM"): // FO4
            case ESM::fourCC("XATP"): // FO4
            case ESM::fourCC("XEMI"): // FO4
            case ESM::fourCC("XFVC"): // FO4
            case ESM::fourCC("XHLT"): // FO4
            case ESM::fourCC("XHTW"): // FO4
            case ESM::fourCC("XLKT"): // FO4
            case ESM::fourCC("XLYR"): // FO4
            case ESM::fourCC("XMBR"): // FO4
            case ESM::fourCC("XMSP"): // FO4
            case ESM::fourCC("XPLK"): // FO4
            case ESM::fourCC("XRFG"): // FO4
            case ESM::fourCC("XRNK"): // FO4
                reader.skipSubRecordData();
                break;
            default:
                throw std::runtime_error("ESM4 ACHR/ACRE load - Unknown subrecord " + ESM::printName(subHdr.typeId));
        }
    }
}

// void ESM4::ActorCharacter::save(ESM4::Writer& writer) const
//{
// }

// void ESM4::ActorCharacter::blank()
//{
// }
