/*
  Copyright (C) 2015-2016, 2018 cc9cii

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
#include "stat.hpp"

#include <stdexcept>
#include <iostream> // FIXME: debug only

#include "reader.hpp"
//#include "writer.hpp"

ESM4::Static::Static() : mFormId(0), mFlags(0), mBoundRadius(0.f)
{
    mEditorId.clear();
    mModel.clear();
}

ESM4::Static::~Static()
{
}

void ESM4::Static::load(ESM4::Reader& reader)
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
            case ESM4::SUB_MODL: reader.getZString(mModel); break;
            case ESM4::SUB_MODB: reader.get(mBoundRadius);  break;
            case ESM4::SUB_MODT:
            {
                // version is only availabe in TES5 (seems to be 27 or 28?)
                //if (reader.esmVersion() == ESM4::VER_094 || reader.esmVersion() == ESM4::VER_170)
                    //std::cout << "STAT MODT ver: " << std::hex << reader.hdr().record.version << std::endl;

                // for TES4 these are just a sequence of bytes
                mMODT.resize(subHdr.dataSize/sizeof(std::uint8_t));
                for (std::vector<std::uint8_t>::iterator it = mMODT.begin(); it != mMODT.end(); ++it)
                {
                    reader.get(*it);
#if 0
                    std::string padding = "";
                    padding.insert(0, reader.stackSize()*2, ' ');
                    std::cout << padding  << "MODT: " << std::hex << *it << std::endl;
#endif
                }
                break;
            }
            case ESM4::SUB_MODS:
            case ESM4::SUB_OBND:
            case ESM4::SUB_DNAM:
            case ESM4::SUB_MNAM:
            case ESM4::SUB_BRUS: // FONV
            case ESM4::SUB_RNAM: // FONV
            {
                //std::cout << "STAT " << ESM4::printName(subHdr.typeId) << " skipping..." << std::endl;
                reader.skipSubRecordData();
                break;
            }
            default:
                throw std::runtime_error("ESM4::STAT::load - Unknown subrecord " + ESM4::printName(subHdr.typeId));
        }
    }
}

//void ESM4::Static::save(ESM4::Writer& writer) const
//{
//}

//void ESM4::Static::blank()
//{
//}
