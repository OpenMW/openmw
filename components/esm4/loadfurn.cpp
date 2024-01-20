/*
  Copyright (C) 2016, 2018, 2021 cc9cii

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
#include "loadfurn.hpp"

#include <stdexcept>

#include "reader.hpp"
//#include "writer.hpp"

void ESM4::Furniture::load(ESM4::Reader& reader)
{
    mId = reader.getFormIdFromHeader();
    mFlags = reader.hdr().record.flags;

    while (reader.getSubRecordHeader())
    {
        const ESM4::SubRecordHeader& subHdr = reader.subRecordHeader();
        switch (subHdr.typeId)
        {
            case ESM4::SUB_EDID:
                reader.getZString(mEditorId);
                break;
            case ESM4::SUB_FULL:
            {
                std::string name;
                reader.getLocalizedString(name);
                // FIXME: subsequent FULL subrecords name object combinations (FO4)
                if (mFullName.empty())
                    mFullName = std::move(name);
                break;
            }
            case ESM4::SUB_MODL:
                reader.getZString(mModel);
                break;
            case ESM4::SUB_SCRI:
                reader.getFormId(mScriptId);
                break;
            case ESM4::SUB_MNAM:
                reader.get(mActiveMarkerFlags);
                break;
            case ESM4::SUB_MODB:
                reader.get(mBoundRadius);
                break;
            case ESM4::SUB_MODT: // Model data
            case ESM4::SUB_MODC:
            case ESM4::SUB_MODS:
            case ESM4::SUB_MODF: // Model data end
            case ESM4::SUB_DAMC: // Destructible
            case ESM4::SUB_DEST:
            case ESM4::SUB_DMDC:
            case ESM4::SUB_DMDL:
            case ESM4::SUB_DMDT:
            case ESM4::SUB_DMDS:
            case ESM4::SUB_DSTA:
            case ESM4::SUB_DSTD:
            case ESM4::SUB_DSTF: // Destructible end
            case ESM4::SUB_ENAM:
            case ESM4::SUB_FNAM:
            case ESM4::SUB_FNMK:
            case ESM4::SUB_FNPR:
            case ESM4::SUB_KNAM:
            case ESM4::SUB_KSIZ:
            case ESM4::SUB_KWDA:
            case ESM4::SUB_NAM0:
            case ESM4::SUB_OBND:
            case ESM4::SUB_PNAM:
            case ESM4::SUB_VMAD:
            case ESM4::SUB_WBDT:
            case ESM4::SUB_XMRK:
            case ESM4::SUB_PRPS:
            case ESM4::SUB_CTDA:
            case ESM4::SUB_CIS1:
            case ESM4::SUB_CIS2:
            case ESM4::SUB_APPR: // FO4
            case ESM4::SUB_ATTX: // FO4
            case ESM4::SUB_CITC: // FO4
            case ESM4::SUB_CNTO: // FO4
            case ESM4::SUB_COCT: // FO4
            case ESM4::SUB_COED: // FO4
            case ESM4::SUB_FTYP: // FO4
            case ESM4::SUB_NAM1: // FO4
            case ESM4::SUB_NTRM: // FO4
            case ESM4::SUB_NVNM: // FO4
            case ESM4::SUB_PTRN: // FO4
            case ESM4::SUB_SNAM: // FO4
            case ESM4::SUB_WNAM: // FO4
            case ESM4::SUB_OBTE: // FO4 object template start
            case ESM4::SUB_OBTF:
            case ESM4::SUB_OBTS:
            case ESM4::SUB_STOP: // FO4 object template end
                reader.skipSubRecordData();
                break;
            default:
                throw std::runtime_error("ESM4::FURN::load - Unknown subrecord " + ESM::printName(subHdr.typeId));
        }
    }
}

// void ESM4::Furniture::save(ESM4::Writer& writer) const
//{
// }

// void ESM4::Furniture::blank()
//{
// }
