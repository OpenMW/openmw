/*
  Copyright (C) 2015-2016, 2018, 2020-2021 cc9cii

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
#include "loadtes4.hpp"

#ifdef NDEBUG // FIXME: debuggigng only
#undef NDEBUG
#endif

#include <cassert>
#include <stdexcept>

#include <iostream> // FIXME: debugging only

#include "common.hpp"
#include "formid.hpp"
#include "reader.hpp"
//#include "writer.hpp"

void ESM4::Header::load(ESM4::Reader& reader)
{
    mFlags = reader.hdr().record.flags; // 0x01 = Rec_ESM, 0x80 = Rec_Localized

    while (reader.getSubRecordHeader())
    {
        const ESM4::SubRecordHeader& subHdr = reader.subRecordHeader();
        switch (subHdr.typeId)
        {
            case ESM4::SUB_HEDR:
            {
                if (!reader.getExact(mData.version) || !reader.getExact(mData.records) || !reader.getExact(mData.nextObjectId))
                    throw std::runtime_error("TES4 HEDR data read error");
                if ((size_t)subHdr.dataSize != sizeof(mData.version)+sizeof(mData.records)+sizeof(mData.nextObjectId))
                    throw std::runtime_error("TES4 HEDR data size mismatch");
                break;
            }
            case ESM4::SUB_CNAM: reader.getZString(mAuthor); break;
            case ESM4::SUB_SNAM: reader.getZString(mDesc); break;
            case ESM4::SUB_MAST: // multiple
            {
                ESM::MasterData m;
                if (!reader.getZString(m.name))
                    throw std::runtime_error("TES4 MAST data read error");

                // NOTE: some mods do not have DATA following MAST so can't read DATA here
                m.size = 0;
                mMaster.push_back (m);
                break;
            }
            case ESM4::SUB_DATA:
            {
                // WARNING: assumes DATA always follows MAST
                if (!reader.getExact(mMaster.back().size))
                    throw std::runtime_error("TES4 DATA data read error");
                break;
            }
            case ESM4::SUB_ONAM:
            {
                mOverrides.resize(subHdr.dataSize/sizeof(FormId));
                for (unsigned int & mOverride : mOverrides)
                {
                    if (!reader.getExact(mOverride))
                        throw std::runtime_error("TES4 ONAM data read error");
#if 0
                    std::string padding;
                    padding.insert(0, reader.stackSize()*2, ' ');
                    std::cout << padding  << "ESM4::Header::ONAM overrides: " << formIdToString(mOverride) << std::endl;
#endif
                }
                break;
            }
            case ESM4::SUB_INTV:
            case ESM4::SUB_INCC:
            case ESM4::SUB_OFST: // Oblivion only?
            case ESM4::SUB_DELE: // Oblivion only?
            {
                //std::cout << ESM::printName(subHdr.typeId) << " skipping..." << std::endl;
                reader.skipSubRecordData();
                break;
            }
            default:
                throw std::runtime_error("ESM4::Header::load - Unknown subrecord " + ESM::printName(subHdr.typeId));
        }
    }
}

//void ESM4::Header::save(ESM4::Writer& writer)
//{
//}
