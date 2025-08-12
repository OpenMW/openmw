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

#include <stdexcept>

#include "common.hpp"
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
            case ESM::fourCC("HEDR"):
            {
                if (!reader.getExact(mData.version) || !reader.getExact(mData.records)
                    || !reader.getExact(mData.nextObjectId))
                    throw std::runtime_error("TES4 HEDR data read error");
                if ((size_t)subHdr.dataSize
                    != sizeof(mData.version) + sizeof(mData.records) + sizeof(mData.nextObjectId))
                    throw std::runtime_error("TES4 HEDR data size mismatch");
                break;
            }
            case ESM::fourCC("CNAM"):
                reader.getZString(mAuthor);
                break;
            case ESM::fourCC("SNAM"):
                reader.getZString(mDesc);
                break;
            case ESM::fourCC("MAST"): // multiple
            {
                ESM::MasterData m;
                if (!reader.getZString(m.name))
                    throw std::runtime_error("TES4 MAST data read error");

                // NOTE: some mods do not have DATA following MAST so can't read DATA here
                m.size = 0;
                mMaster.push_back(std::move(m));
                break;
            }
            case ESM::fourCC("DATA"):
            {
                if (mMaster.empty())
                    throw std::runtime_error(
                        "Failed to read TES4 DATA subrecord: there is no preceding MAST subrecord");
                // WARNING: assumes DATA always follows MAST
                if (!reader.getExact(mMaster.back().size))
                    throw std::runtime_error("TES4 DATA data read error");
                break;
            }
            case ESM::fourCC("ONAM"):
            {
                mOverrides.resize(subHdr.dataSize / sizeof(ESM::FormId32));
                for (ESM::FormId& mOverride : mOverrides)
                {
                    uint32_t v;
                    if (!reader.getExact(v))
                        throw std::runtime_error("TES4 ONAM data read error");
                    mOverride = ESM::FormId::fromUint32(v);
#if 0
                    std::string padding;
                    padding.insert(0, reader.stackSize()*2, ' ');
                    std::cout << padding  << "ESM4::Header::ONAM overrides: " << formIdToString(mOverride) << std::endl;
#endif
                }
                break;
            }
            case ESM::fourCC("INTV"):
            case ESM::fourCC("INCC"):
            case ESM::fourCC("OFST"): // Oblivion only?
            case ESM::fourCC("DELE"): // Oblivion only?
            case ESM::fourCC("TNAM"): // Fallout 4 (CK only)
            case ESM::fourCC("MMSB"): // Fallout 76
                reader.skipSubRecordData();
                break;
            default:
                throw std::runtime_error("ESM4::Header::load - Unknown subrecord " + ESM::printName(subHdr.typeId));
        }
    }
}

// void ESM4::Header::save(ESM4::Writer& writer)
//{
// }
