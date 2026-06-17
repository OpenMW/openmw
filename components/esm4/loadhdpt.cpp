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

#include <cstdint>
#include <optional>
#include <stdexcept>

#include "reader.hpp"
//#include "writer.hpp"

void ESM4::HeadPart::load(ESM4::Reader& reader)
{
    mId = reader.getFormIdFromHeader();
    mFlags = reader.hdr().record.flags;
    mExtraFlags2 = 0;
    mData = 0;
    mType = 0;

    std::optional<std::uint32_t> type;

    while (reader.getSubRecordHeader())
    {
        const ESM4::SubRecordHeader& subHdr = reader.subRecordHeader();
        switch (subHdr.typeId)
        {
            case ESM::fourCC("EDID"):
                reader.getZString(mEditorId);
                break;
            case ESM::fourCC("XALG"): // FO76
                reader.get(mExtraFlags2);
                break;
            case ESM::fourCC("FULL"):
                reader.getLocalizedString(mFullName);
                break;
            case ESM::fourCC("DATA"):
                reader.get(mData);
                break;
            case ESM::fourCC("MODL"):
                reader.getZString(mModel);
                break;
            case ESM::fourCC("HNAM"):
                reader.getFormId(mExtraParts.emplace_back());
                break;
            case ESM::fourCC("NAM0"): // TES5
            {
                std::uint32_t value;
                reader.get(value);
                type = value;
                break;
            }
            case ESM::fourCC("NAM1"): // TES5
            {
                std::string file;
                reader.getZString(file);

                if (!type.has_value())
                    throw std::runtime_error(
                        "Failed to read ESM4 HDPT record: subrecord NAM0 does not precede subrecord NAM1: file type is "
                        "unknown");

                if (*type >= mTriFile.size())
                    throw std::runtime_error(
                        "Failed to read ESM4 HDPT record: invalid file type: " + std::to_string(*type));

                mTriFile[*type] = std::move(file);
                break;
            }
            case ESM::fourCC("TNAM"):
                reader.getFormId(mBaseTexture);
                break;
            case ESM::fourCC("CNAM"):
                reader.getFormId(mColor);
                break;
            case ESM::fourCC("RNAM"):
                reader.getFormId(mValidRaces.emplace_back());
                break;
            case ESM::fourCC("PNAM"):
                reader.get(mType);
                break;
            case ESM::fourCC("MODT"): // Model data
            case ESM::fourCC("MODC"):
            case ESM::fourCC("MODS"):
            case ESM::fourCC("MODF"):
            case ESM::fourCC("ENLM"):
            case ESM::fourCC("XFLG"):
            case ESM::fourCC("ENLT"):
            case ESM::fourCC("ENLS"):
            case ESM::fourCC("AUUV"):
            case ESM::fourCC("MODD"): // Model data end
            case ESM::fourCC("CTDA"):
                reader.skipSubRecordData();
                break;
            default:
                throw std::runtime_error("ESM4::HDPT::load - Unknown subrecord " + ESM::printName(subHdr.typeId));
        }
    }
}

// void ESM4::HeadPart::save(ESM4::Writer& writer) const
//{
// }

// void ESM4::HeadPart::blank()
//{
// }
