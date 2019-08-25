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
#include "txst.hpp"

#include <stdexcept>
#include <iostream> // FIXME: testing only

#include "reader.hpp"
//#include "writer.hpp"

ESM4::TextureSet::TextureSet() : mFormId(0), mFlags(0)
{
    mEditorId.clear();
    mColorMap.clear();
    mNormalMap.clear();
    mEnvMask.clear();
    mToneMap.clear();
    mDetailMap.clear();
    mEnvMap.clear();
    mUnknown.clear();
    mSpecular.clear();
}

ESM4::TextureSet::~TextureSet()
{
}

void ESM4::TextureSet::load(ESM4::Reader& reader)
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
            case ESM4::SUB_TX00: reader.getZString(mColorMap); break;
            case ESM4::SUB_TX01: reader.getZString(mNormalMap); break;
            case ESM4::SUB_TX02: reader.getZString(mEnvMask); break;
            case ESM4::SUB_TX03: reader.getZString(mToneMap); break;
            case ESM4::SUB_TX04: reader.getZString(mDetailMap); break;
            case ESM4::SUB_TX05: reader.getZString(mEnvMap); break;
            case ESM4::SUB_TX06: reader.getZString(mUnknown); break;
            case ESM4::SUB_TX07: reader.getZString(mSpecular); break;
            case ESM4::SUB_DNAM:
            case ESM4::SUB_DODT:
            case ESM4::SUB_OBND: // object bounds
            {
                //std::cout << "TXST " << ESM4::printName(subHdr.typeId) << " skipping..." << std::endl;
                reader.skipSubRecordData();
                break;
            }
            default:
                throw std::runtime_error("ESM4::TXST::load - Unknown subrecord " + ESM4::printName(subHdr.typeId));
        }
    }
}

//void ESM4::TextureSet::save(ESM4::Writer& writer) const
//{
//}

//void ESM4::TextureSet::blank()
//{
//}
