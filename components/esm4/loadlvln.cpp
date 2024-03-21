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
#include "loadlvln.hpp"

#include <stdexcept>

#include "reader.hpp"
//#include "writer.hpp"

void ESM4::LevelledNpc::load(ESM4::Reader& reader)
{
    mId = reader.getFormIdFromHeader();
    mFlags = reader.hdr().record.flags;
    // std::uint32_t esmVer = reader.esmVersion(); // currently unused

    while (reader.getSubRecordHeader())
    {
        const ESM4::SubRecordHeader& subHdr = reader.subRecordHeader();
        switch (subHdr.typeId)
        {
            case ESM::fourCC("EDID"):
                reader.getZString(mEditorId);
                break;
            case ESM::fourCC("MODL"):
                reader.getZString(mModel);
                break;
            case ESM::fourCC("LLCT"):
                reader.get(mListCount);
                break;
            case ESM::fourCC("LVLD"):
                reader.get(mChanceNone);
                break;
            case ESM::fourCC("LVLF"):
                reader.get(mLvlActorFlags);
                break;
            case ESM::fourCC("LVLO"):
            {
                LVLO lvlo;
                if (subHdr.dataSize != 12)
                {
                    if (subHdr.dataSize == 8)
                    {
                        reader.get(lvlo.level);
                        reader.get(lvlo.item);
                        reader.get(lvlo.count);
                        break;
                    }
                    else
                        throw std::runtime_error("ESM4::LVLN::load - " + mEditorId + " LVLO size error");
                }
                //              else if (esmVer == ESM::VER_094 || esmVer == ESM::VER_170 || isFONV)
                //              {
                //                  std::uint32_t level;
                //                  reader.get(level);
                //                  lvlo.level = static_cast<std::uint16_t>(level);
                //                  reader.get(lvlo.item);
                //                  std::uint32_t count;
                //                  reader.get(count);
                //                  lvlo.count = static_cast<std::uint16_t>(count);
                //              }
                else
                    reader.get(lvlo);

                reader.adjustFormId(lvlo.item);
                mLvlObject.push_back(lvlo);
                break;
            }
            case ESM::fourCC("COED"): // owner
            case ESM::fourCC("OBND"): // object bounds
            case ESM::fourCC("MODT"): // Model data
            case ESM::fourCC("MODC"):
            case ESM::fourCC("MODS"):
            case ESM::fourCC("MODF"): // Model data end
            case ESM::fourCC("LLKC"): // FO4
            case ESM::fourCC("LVLG"): // FO4
            case ESM::fourCC("LVLM"): // FO4
                reader.skipSubRecordData();
                break;
            default:
                throw std::runtime_error("ESM4::LVLN::load - Unknown subrecord " + ESM::printName(subHdr.typeId));
        }
    }
}

// void ESM4::LevelledNpc::save(ESM4::Writer& writer) const
//{
// }

// void ESM4::LevelledNpc::blank()
//{
// }
