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
#include "loadhdpt.hpp"

#include <stdexcept>
#include <optional>
//#include <iostream> // FIXME: testing only

#include "reader.hpp"
//#include "writer.hpp"

void ESM4::HeadPart::load(ESM4::Reader& reader)
{
    mFormId = reader.hdr().record.id;
    reader.adjustFormId(mFormId);
    mFlags  = reader.hdr().record.flags;

    std::optional<std::uint32_t> type;

    while (reader.getSubRecordHeader())
    {
        const ESM4::SubRecordHeader& subHdr = reader.subRecordHeader();
        switch (subHdr.typeId)
        {
            case ESM4::SUB_EDID: reader.getZString(mEditorId); break;
            case ESM4::SUB_FULL: reader.getLocalizedString(mFullName); break;
            case ESM4::SUB_DATA: reader.get(mData); break;
            case ESM4::SUB_MODL: reader.getZString(mModel); break;
            case ESM4::SUB_HNAM: reader.getFormId(mAdditionalPart); break;
            case ESM4::SUB_NAM0: // TES5
            {
                std::uint32_t value;
                reader.get(value);
                type = value;

                break;
            }
            case ESM4::SUB_NAM1: // TES5
            {
                std::string file;
                reader.getZString(file);

                if (!type.has_value())
                    throw std::runtime_error("Failed to read ESM4 HDPT record: subrecord NAM0 does not precede subrecord NAM1: file type is unknown");

                if (*type >= mTriFile.size())
                    throw std::runtime_error("Failed to read ESM4 HDPT record: invalid file type: " + std::to_string(*type));

                mTriFile[*type] = std::move(file);

                break;
            }
            case ESM4::SUB_TNAM: reader.getFormId(mBaseTexture); break;
            case ESM4::SUB_PNAM:
            case ESM4::SUB_MODS:
            case ESM4::SUB_MODT:
            case ESM4::SUB_RNAM:
            {
                //std::cout << "HDPT " << ESM::printName(subHdr.typeId) << " skipping..." << std::endl;
                reader.skipSubRecordData();
                break;
            }
            default:
                throw std::runtime_error("ESM4::HDPT::load - Unknown subrecord " + ESM::printName(subHdr.typeId));
        }
    }
}

//void ESM4::HeadPart::save(ESM4::Writer& writer) const
//{
//}

//void ESM4::HeadPart::blank()
//{
//}
