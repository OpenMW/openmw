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
#include "idlm.hpp"

#include <stdexcept>
//#include <iostream> // FIXME: testing only

#include "reader.hpp"
//#include "writer.hpp"

ESM4::IdleMarker::IdleMarker() : mFormId(0), mFlags(0), mIdleFlags(0), mIdleCount(0), mIdleTimer(0.f), mIdleAnim(0)
{
    mEditorId.clear();
}

ESM4::IdleMarker::~IdleMarker()
{
}

void ESM4::IdleMarker::load(ESM4::Reader& reader)
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
            case ESM4::SUB_IDLF: reader.get(mIdleFlags);       break;
            case ESM4::SUB_IDLC:
            {
                if (subHdr.dataSize != 1) // FO3 can have 4?
                {
                    reader.skipSubRecordData();
                    break;
                }

                reader.get(mIdleCount);
                break;
            }
            case ESM4::SUB_IDLT: reader.get(mIdleTimer);       break;
            case ESM4::SUB_IDLA:
            {
                if (esmVer == ESM4::VER_094) // FO3? 4 or 8 bytes
                {
                    reader.skipSubRecordData();
                    break;
                }

                mIdleAnim.resize(mIdleCount);
                for (unsigned int i = 0; i < static_cast<unsigned int>(mIdleCount); ++i)
                    reader.get(mIdleAnim.at(i));
                break;
            }
            case ESM4::SUB_OBND: // object bounds
            {
                //std::cout << "IDLM " << ESM4::printName(subHdr.typeId) << " skipping..." << std::endl;
                reader.skipSubRecordData();
                break;
            }
            default:
                throw std::runtime_error("ESM4::IDLM::load - Unknown subrecord " + ESM4::printName(subHdr.typeId));
        }
    }
}

//void ESM4::IdleMarker::save(ESM4::Writer& writer) const
//{
//}

//void ESM4::IdleMarker::blank()
//{
//}
