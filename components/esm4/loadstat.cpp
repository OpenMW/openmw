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
#include "loadstat.hpp"

#include <stdexcept>

#include "reader.hpp"
// #include "writer.hpp"

void ESM4::Static::load(ESM4::Reader& reader)
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
            case ESM::fourCC("MODL"):
                reader.getZString(mModel);
                break;
            case ESM::fourCC("MODB"):
                reader.get(mBoundRadius);
                break;
            case ESM::fourCC("MODT"):
            {
                // version is only availabe in TES5 (seems to be 27 or 28?)
                // if (reader.esmVersion() == ESM::VER_094 || reader.esmVersion() == ESM::VER_170)
                // std::cout << "STAT MODT ver: " << std::hex << reader.hdr().record.version << std::endl;

                // for TES4 these are just a sequence of bytes
                mMODT.resize(subHdr.dataSize / sizeof(std::uint8_t));
                for (std::vector<std::uint8_t>::iterator it = mMODT.begin(); it != mMODT.end(); ++it)
                {
                    reader.get(*it);
#if 0
                    std::string padding;
                    padding.insert(0, reader.stackSize()*2, ' ');
                    std::cout << padding  << "MODT: " << std::hex << *it << std::endl;
#endif
                }
                break;
            }
            case ESM::fourCC("MNAM"):
            {
                for (std::string& level : mLOD)
                {
                    level.resize(260);
                    reader.get(level.data(), 260);
                    size_t end = level.find('\0');
                    if (end != std::string::npos)
                        level.erase(end);
                }
                break;
            }
            case ESM::fourCC("MODC"): // More model data
            case ESM::fourCC("MODS"):
            case ESM::fourCC("MODF"): // Model data end
            case ESM::fourCC("OBND"):
            case ESM::fourCC("DNAM"):
            case ESM::fourCC("BRUS"): // FONV
            case ESM::fourCC("RNAM"): // FONV
            case ESM::fourCC("FTYP"): // FO4
            case ESM::fourCC("NVNM"): // FO4
            case ESM::fourCC("PRPS"): // FO4
            case ESM::fourCC("PTRN"): // FO4
            case ESM::fourCC("VMAD"): // FO4
                reader.skipSubRecordData();
                break;
            default:
                throw std::runtime_error("ESM4::STAT::load - Unknown subrecord " + ESM::printName(subHdr.typeId));
        }
    }
}

// void ESM4::Static::save(ESM4::Writer& writer) const
//{
// }

// void ESM4::Static::blank()
//{
// }
