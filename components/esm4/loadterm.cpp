/*
  Copyright (C) 2019-2021 cc9cii

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
#include "loadterm.hpp"

#include <stdexcept>

#include "reader.hpp"
// #include "writer.hpp"

void ESM4::Terminal::load(ESM4::Reader& reader)
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
            case ESM::fourCC("FULL"):
                reader.getLocalizedString(mFullName);
                break;
            case ESM::fourCC("DESC"):
                reader.getLocalizedString(mText);
                break;
            case ESM::fourCC("SCRI"):
                reader.getFormId(mScriptId);
                break;
            case ESM::fourCC("PNAM"):
                reader.getFormId(mPasswordNote);
                break;
            case ESM::fourCC("SNAM"):
                if (subHdr.dataSize == 4)
                    reader.getFormId(mSound);
                // FIXME: FO4 sound marker params
                else
                    reader.skipSubRecordData();
                break;
            case ESM::fourCC("MODL"):
                reader.getZString(mModel);
                break;
            case ESM::fourCC("RNAM"):
                reader.getZString(mResultText);
                break;
            case ESM::fourCC("DNAM"): // difficulty
            case ESM::fourCC("ANAM"): // flags
            case ESM::fourCC("CTDA"):
            case ESM::fourCC("CIS1"):
            case ESM::fourCC("CIS2"):
            case ESM::fourCC("INAM"):
            case ESM::fourCC("ITXT"): // Menu Item
            case ESM::fourCC("MODT"): // Model data
            case ESM::fourCC("MODC"):
            case ESM::fourCC("MODS"):
            case ESM::fourCC("MODF"): // Model data end
            case ESM::fourCC("SCDA"):
            case ESM::fourCC("SCHR"):
            case ESM::fourCC("SCRO"):
            case ESM::fourCC("SCRV"):
            case ESM::fourCC("SCTX"):
            case ESM::fourCC("SCVR"):
            case ESM::fourCC("SLSD"):
            case ESM::fourCC("TNAM"):
            case ESM::fourCC("OBND"):
            case ESM::fourCC("VMAD"):
            case ESM::fourCC("KSIZ"):
            case ESM::fourCC("KWDA"):
            case ESM::fourCC("BSIZ"): // FO4
            case ESM::fourCC("BTXT"): // FO4
            case ESM::fourCC("COCT"): // FO4
            case ESM::fourCC("CNTO"): // FO4
            case ESM::fourCC("FNAM"): // FO4
            case ESM::fourCC("ISIZ"): // FO4
            case ESM::fourCC("ITID"): // FO4
            case ESM::fourCC("MNAM"): // FO4
            case ESM::fourCC("NAM0"): // FO4
            case ESM::fourCC("PRPS"): // FO4
            case ESM::fourCC("PTRN"): // FO4
            case ESM::fourCC("UNAM"): // FO4
            case ESM::fourCC("VNAM"): // FO4
            case ESM::fourCC("WBDT"): // FO4
            case ESM::fourCC("WNAM"): // FO4
            case ESM::fourCC("XMRK"): // FO4
                reader.skipSubRecordData();
                break;
            default:
                throw std::runtime_error("ESM4::TERM::load - Unknown subrecord " + ESM::printName(subHdr.typeId));
        }
    }
}

// void ESM4::Terminal::save(ESM4::Writer& writer) const
//{
// }

// void ESM4::Terminal::blank()
//{
// }
