/*
  Copyright (C) 2016, 2018, 2020-2021 cc9cii

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
#include "loadligh.hpp"

#include <stdexcept>

#include "reader.hpp"
//#include "writer.hpp"

void ESM4::Light::load(ESM4::Reader& reader)
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
            case ESM::fourCC("DATA"):
            {
                // 24: older TES4 records (found in GOG release)
                // 32: newer TES4 records, FO3/FNV
                // 48: TES5
                // 64: FO4/FO76
                if (subHdr.dataSize != 24 && subHdr.dataSize != 32 && subHdr.dataSize != 48 && subHdr.dataSize != 64)
                {
                    reader.skipSubRecordData();
                    break;
                }
                reader.get(mData.time);
                reader.get(mData.radius);
                reader.get(mData.colour);
                reader.get(mData.flags);
                if (subHdr.dataSize >= 32)
                {
                    reader.get(mData.falloff);
                    reader.get(mData.FOV);
                    if (subHdr.dataSize >= 48)
                    {
                        reader.get(mData.nearClip);
                        reader.get(mData.frequency);
                        reader.get(mData.intensityAmplitude);
                        reader.get(mData.movementAmplitude);
                        if (subHdr.dataSize == 64)
                        {
                            reader.get(mData.constant);
                            reader.get(mData.scalar);
                            reader.get(mData.exponent);
                            reader.get(mData.godRaysNearClip);
                        }
                    }
                }
                reader.get(mData.value);
                reader.get(mData.weight);
                break;
            }
            case ESM::fourCC("MODL"):
                reader.getZString(mModel);
                break;
            case ESM::fourCC("ICON"):
                reader.getZString(mIcon);
                break;
            case ESM::fourCC("SCRI"):
                reader.getFormId(mScriptId);
                break;
            case ESM::fourCC("SNAM"):
                reader.getFormId(mSound);
                break;
            case ESM::fourCC("MODB"):
                reader.get(mBoundRadius);
                break;
            case ESM::fourCC("FNAM"):
                reader.get(mFade);
                break;
            case ESM::fourCC("MODT"): // Model data
            case ESM::fourCC("MODC"):
            case ESM::fourCC("MODS"):
            case ESM::fourCC("MODF"): // Model data end
            case ESM::fourCC("OBND"):
            case ESM::fourCC("VMAD"):
            case ESM::fourCC("DAMC"): // Destructible
            case ESM::fourCC("DEST"):
            case ESM::fourCC("DMDC"):
            case ESM::fourCC("DMDL"):
            case ESM::fourCC("DMDT"):
            case ESM::fourCC("DMDS"):
            case ESM::fourCC("DSTA"):
            case ESM::fourCC("DSTD"):
            case ESM::fourCC("DSTF"): // Destructible end
            case ESM::fourCC("KSIZ"):
            case ESM::fourCC("KWDA"):
            case ESM::fourCC("LNAM"): // FO4
            case ESM::fourCC("MICO"): // FO4
            case ESM::fourCC("NAM0"): // FO4
            case ESM::fourCC("PRPS"): // FO4
            case ESM::fourCC("PTRN"): // FO4
            case ESM::fourCC("WGDR"): // FO4
                reader.skipSubRecordData();
                break;
            default:
                throw std::runtime_error("ESM4::LIGH::load - Unknown subrecord " + ESM::printName(subHdr.typeId));
        }
    }
}

// void ESM4::Light::save(ESM4::Writer& writer) const
//{
// }

// void ESM4::Light::blank()
//{
// }
