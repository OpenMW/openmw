/*
  Copyright (C) 2016, 2018 cc9cii

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
#include "ligh.hpp"

#include <stdexcept>

#include "reader.hpp"
//#include "writer.hpp"

ESM4::Light::Light() : mFormId(0), mFlags(0), mBoundRadius(0.f), mScript(0), mSound(0),
                       mFade(0.f)
{
    mEditorId.clear();
    mFullName.clear();
    mModel.clear();
    mIcon.clear();
}

ESM4::Light::~Light()
{
}

void ESM4::Light::load(ESM4::Reader& reader)
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
            case ESM4::SUB_FULL:
            {
                if (reader.hasLocalizedStrings())
                    reader.getLocalizedString(mFullName);
                else if (!reader.getZString(mFullName))
                    throw std::runtime_error ("LIGH FULL data read error");

                break;
            }
            case ESM4::SUB_DATA:
            {
                if (isFONV || (esmVer == ESM4::VER_094 && subHdr.dataSize == 32)/*FO3*/)
                {
                    reader.get(mData.time);     // uint32
                }
                else
                    reader.get(mData.duration); // float

                reader.get(mData.radius);
                reader.get(mData.colour);
                reader.get(mData.flags);
                //if (reader.esmVersion() == ESM4::VER_094 || reader.esmVersion() == ESM4::VER_170)
                if (subHdr.dataSize == 48)
                {
                    reader.get(mData.falloff);
                    reader.get(mData.FOV);
                    reader.get(mData.nearClip);
                    reader.get(mData.frequency);
                    reader.get(mData.intensityAmplitude);
                    reader.get(mData.movementAmplitude);
                }
                else if (subHdr.dataSize == 32)
                {
                    reader.get(mData.falloff);
                    reader.get(mData.FOV);
                }
                reader.get(mData.value);
                reader.get(mData.weight);
                break;
            }
            case ESM4::SUB_MODL: reader.getZString(mModel); break;
            case ESM4::SUB_ICON: reader.getZString(mIcon);  break;
            case ESM4::SUB_SCRI: reader.getFormId(mScript); break;
            case ESM4::SUB_SNAM: reader.getFormId(mSound);  break;
            case ESM4::SUB_MODB: reader.get(mBoundRadius);  break;
            case ESM4::SUB_FNAM: reader.get(mFade);         break;
            case ESM4::SUB_MODT:
            case ESM4::SUB_OBND:
            case ESM4::SUB_VMAD: // Dragonborn only?
            {
                //std::cout << "LIGH " << ESM4::printName(subHdr.typeId) << " skipping..." << std::endl;
                reader.skipSubRecordData();
                break;
            }
            default:
                throw std::runtime_error("ESM4::LIGH::load - Unknown subrecord " + ESM4::printName(subHdr.typeId));
        }
    }
}

//void ESM4::Light::save(ESM4::Writer& writer) const
//{
//}

//void ESM4::Light::blank()
//{
//}
