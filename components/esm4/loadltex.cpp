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
#include "loadltex.hpp"

#include <stdexcept>

#include "reader.hpp"
//#include "writer.hpp"

void ESM4::LandTexture::load(ESM4::Reader& reader)
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
            case ESM::fourCC("HNAM"):
            {
                switch (subHdr.dataSize)
                {
                    case 3: // TES4, FO3, FNV
                        reader.get(mHavokMaterial);
                        [[fallthrough]];
                    case 2:
                        reader.get(mHavokFriction);
                        reader.get(mHavokRestitution);
                        break;
                    default:
                        reader.skipSubRecordData();
                        break;
                }
                break;
            }
            case ESM::fourCC("ICON"):
                reader.getZString(mTextureFile);
                break; // Oblivion only?
            case ESM::fourCC("SNAM"):
                reader.get(mTextureSpecular);
                break;
            case ESM::fourCC("GNAM"):
                reader.getFormId(mGrass.emplace_back());
                break;
            case ESM::fourCC("TNAM"):
                reader.getFormId(mTexture);
                break; // TES5, FO4
            case ESM::fourCC("MNAM"):
                reader.getFormId(mMaterial);
                break; // TES5, FO4
            case ESM::fourCC("INAM"):
                reader.get(mMaterialFlags);
                break; // SSE
            default:
                throw std::runtime_error("ESM4::LTEX::load - Unknown subrecord " + ESM::printName(subHdr.typeId));
        }
    }
}

// void ESM4::LandTexture::save(ESM4::Writer& writer) const
//{
// }

// void ESM4::LandTexture::blank()
//{
// }
