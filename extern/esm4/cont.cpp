/*
  Copyright (C) 2016, 2018 cc9cii

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
#include "cont.hpp"

#include <stdexcept>

#include "reader.hpp"
//#include "writer.hpp"

ESM4::Container::Container() : mFormId(0), mFlags(0), mBoundRadius(0.f), mDataFlags(0), mWeight(0.f),
                               mOpenSound(0), mCloseSound(0), mScript(0)
{
    mEditorId.clear();
    mFullName.clear();
    mModel.clear();
}

ESM4::Container::~Container()
{
}

void ESM4::Container::load(ESM4::Reader& reader)
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
            case ESM4::SUB_FULL:
            {
                if (reader.hasLocalizedStrings())
                    reader.getLocalizedString(mFullName);
                else if (!reader.getZString(mFullName))
                    throw std::runtime_error ("CONT FULL data read error");

                break;
            }
            case ESM4::SUB_DATA:
            {
                reader.get(mDataFlags);
                reader.get(mWeight);
                break;
            }
            case ESM4::SUB_CNTO:
            {
                static InventoryItem inv; // FIXME: use unique_ptr here?
                reader.get(inv);
                reader.adjustFormId(inv.item);
                mInventory.push_back(inv);
                break;
            }
            case ESM4::SUB_MODL: reader.getZString(mModel);     break;
            case ESM4::SUB_SNAM: reader.getFormId(mOpenSound);  break;
            case ESM4::SUB_QNAM: reader.getFormId(mCloseSound); break;
            case ESM4::SUB_SCRI: reader.getFormId(mScript);     break;
            case ESM4::SUB_MODB: reader.get(mBoundRadius);      break;
            case ESM4::SUB_MODT:
            case ESM4::SUB_MODS: // TES5 only
            case ESM4::SUB_VMAD: // TES5 only
            case ESM4::SUB_OBND: // TES5 only
            case ESM4::SUB_COCT: // TES5 only
            case ESM4::SUB_COED: // TES5 only
            case ESM4::SUB_DEST: // FONV
            case ESM4::SUB_DSTD: // FONV
            case ESM4::SUB_DSTF: // FONV
            case ESM4::SUB_DMDL: // FONV
            case ESM4::SUB_DMDT: // FONV
            case ESM4::SUB_RNAM: // FONV
            {
                //std::cout << "CONT " << ESM4::printName(subHdr.typeId) << " skipping..." << std::endl;
                reader.skipSubRecordData();
                break;
            }
            default:
                throw std::runtime_error("ESM4::CONT::load - Unknown subrecord " + ESM4::printName(subHdr.typeId));
        }
    }
}

//void ESM4::Container::save(ESM4::Writer& writer) const
//{
//}

//void ESM4::Container::blank()
//{
//}
