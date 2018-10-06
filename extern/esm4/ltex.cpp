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
#include "ltex.hpp"

#include <cassert>
#include <stdexcept>

#include "reader.hpp"
//#include "writer.hpp"

#ifdef NDEBUG // FIXME: debuggigng only
#undef NDEBUG
#endif

ESM4::LandTexture::LandTexture() : mFormId(0), mFlags(0), mHavokFriction(0), mHavokRestitution(0),
                                   mTextureSpecular(0), mGrass(0), mHavokMaterial(0), mTexture(0),
                                   mMaterial(0)
{
    mEditorId.clear();
    mTextureFile.clear();
}

ESM4::LandTexture::~LandTexture()
{
}

void ESM4::LandTexture::load(ESM4::Reader& reader)
{
    mFormId = reader.hdr().record.id;
    reader.adjustFormId(mFormId);
    mFlags  = reader.hdr().record.flags;
    std::uint32_t esmVer = reader.esmVersion();
    bool isFONV = esmVer == ESM4::VER_132 || esmVer == ESM4::VER_133 || esmVer == ESM4::VER_134;

    while (reader.getSubRecordHeader())
    {
        const ESM4::SubRecordHeader& subHdr = reader.subRecordHeader();
        switch (subHdr.typeId)
        {
            case ESM4::SUB_EDID: reader.getZString(mEditorId); break;
            case ESM4::SUB_HNAM:
            {
                if (isFONV)
                {
                    reader.skipSubRecordData(); // FIXME: skip FONV for now
                    break;
                }

                if ((reader.esmVersion() == ESM4::VER_094 || reader.esmVersion() == ESM4::VER_170)
                    && subHdr.dataSize == 2) // FO3 is VER_094 but dataSize 3
                {
                    //assert(subHdr.dataSize == 2 && "LTEX unexpected HNAM size");
                    reader.get(mHavokFriction);
                    reader.get(mHavokRestitution);
                }
                else
                {
                    assert(subHdr.dataSize == 3 && "LTEX unexpected HNAM size");
                    reader.get(mHavokMaterial);
                    reader.get(mHavokFriction);
                    reader.get(mHavokRestitution);
                }
                break;
            }
            case ESM4::SUB_ICON: reader.getZString(mTextureFile); break; // Oblivion only?
            case ESM4::SUB_SNAM: reader.get(mTextureSpecular); break;
            case ESM4::SUB_GNAM: reader.getFormId(mGrass);     break;
            case ESM4::SUB_TNAM: reader.getFormId(mTexture);   break; // TES5 only
            case ESM4::SUB_MNAM: reader.getFormId(mMaterial);  break; // TES5 only
            default:
                throw std::runtime_error("ESM4::LTEX::load - Unknown subrecord " + ESM4::printName(subHdr.typeId));
        }
    }
}

//void ESM4::LandTexture::save(ESM4::Writer& writer) const
//{
//}

//void ESM4::LandTexture::blank()
//{
//}
