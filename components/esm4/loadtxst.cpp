/*
  Copyright (C) 2019, 2020 cc9cii

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
#include "loadtxst.hpp"

#include <stdexcept>

#include "reader.hpp"
//#include "writer.hpp"

void ESM4::TextureSet::load(ESM4::Reader& reader)
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
            case ESM::fourCC("FLTR"): // FO76
                reader.getZString(mFilter);
                break;
            case ESM::fourCC("TX00"):
                reader.getZString(mDiffuse);
                break;
            case ESM::fourCC("TX01"):
                reader.getZString(mNormalMap);
                break;
            case ESM::fourCC("TX02"):
                // This is a "wrinkle map" in FO4/76
                reader.getZString(mEnvMask);
                break;
            case ESM::fourCC("TX03"):
                // This is a glow map in FO4/76
                reader.getZString(mToneMap);
                break;
            case ESM::fourCC("TX04"):
                // This is a height map in FO4/76
                reader.getZString(mDetailMap);
                break;
            case ESM::fourCC("TX05"):
                reader.getZString(mEnvMap);
                break;
            case ESM::fourCC("TX06"):
                reader.getZString(mMultiLayer);
                break;
            case ESM::fourCC("TX07"):
                // This is a "smooth specular" map in FO4/76
                reader.getZString(mSpecular);
                break;
            case ESM::fourCC("TX08"): // FO76
                reader.getZString(mSpecular);
                break;
            case ESM::fourCC("TX09"): // FO76
                reader.getZString(mLighting);
                break;
            case ESM::fourCC("TX10"): // FO76
                reader.getZString(mFlow);
                break;
            case ESM::fourCC("DNAM"):
                reader.get(mDataFlags);
                break;
            case ESM::fourCC("MNAM"):
                reader.getZString(mMaterial);
                break;
            case ESM::fourCC("DODT"): // Decal data
            case ESM::fourCC("OBND"): // object bounds
            case ESM::fourCC("OPDS"): // Object placement defaults, FO76
                reader.skipSubRecordData();
                break;
            default:
                throw std::runtime_error("ESM4::TXST::load - Unknown subrecord " + ESM::printName(subHdr.typeId));
        }
    }
}

// void ESM4::TextureSet::save(ESM4::Writer& writer) const
//{
// }

// void ESM4::TextureSet::blank()
//{
// }
