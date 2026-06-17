/*
  Copyright (C) 2015 - 2024 cc9cii

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

  cc9cii cc9cii@hotmail.com

  Much of the information on the data structures are based on the information
  from Tes4Mod:Mod_File_Format and Tes5Mod:File_Formats but also refined by
  trial & error.  See http://en.uesp.net/wiki for details.

*/
#include "loadland.hpp"

#include <cassert>
#include <cstdint>
#include <stdexcept>

#include <components/debug/debuglog.hpp>

#include "reader.hpp"

namespace
{
    void assignDefaultTextures(ESM4::Land& land, ESM4::Reader& reader)
    {
        std::uint32_t esmVer = reader.esmVersion();

        // Note: in games after TES4 it can be configured in ini file (sDefaultLandDiffuseTexture)
        if (!reader.hasFormVersion() && (esmVer == ESM::VER_080 || esmVer == ESM::VER_100)) // TES4
        {
            land.mDefaultDiffuseMap = VFS::Path::NormalizedView("textures/landscape/terrainhddirt01.dds");
            land.mDefaultNormalMap = VFS::Path::NormalizedView("textures/landscape/terrainhddirt01_n.dds");
        }
        else if (reader.hasFormVersion() && reader.formVersion() >= 16
            && (esmVer == ESM::VER_094 || esmVer == ESM::VER_170 || esmVer == ESM::VER_171)) // TES5
        {
            land.mDefaultDiffuseMap = VFS::Path::NormalizedView("textures/landscape/dirt02.dds");
            land.mDefaultNormalMap = VFS::Path::NormalizedView("textures/landscape/dirt02_n.dds");
        }
        else if (esmVer == ESM::VER_095 || esmVer == ESM::VER_100) // FO4
        {
            land.mDefaultDiffuseMap
                = VFS::Path::NormalizedView("textures/landscape/ground/commonwealthdefault01_d.dds");
            land.mDefaultNormalMap = VFS::Path::NormalizedView("textures/landscape/ground/commonwealthdefault01_n.dds");
        }
        else if (esmVer == ESM::VER_094 || esmVer == ESM::VER_132 || esmVer == ESM::VER_133 || esmVer == ESM::VER_134)
        { // FO3, FONV
            land.mDefaultDiffuseMap = VFS::Path::NormalizedView("textures/landscape/dirtwasteland01.dds");
            land.mDefaultNormalMap = VFS::Path::NormalizedView("textures/landscape/dirtwasteland01_n.dds");
        }
        else
        {
            // Nothing especially bad happens if default texture is not set (except of the missing texture of course),
            // but we throw an error because this case is unexpected and detection logic needs to be updated.
            throw std::runtime_error("ESM4::Land unknown ESM version");
        }
    }
}

//             overlap north
//
//         32
//         31
//         30
// overlap  .
//  west    .
//          .
//          2
//          1
//          0
//           0 1 2 ... 30 31 32
//
//             overlap south
//
void ESM4::Land::load(ESM4::Reader& reader)
{
    mId = reader.getFormIdFromHeader();
    mFlags = reader.hdr().record.flags;

    mDataTypes = 0;
    mCell = reader.currCell();
    TxtLayer layer;
    std::int8_t currentAddQuad = -1; // for VTXT following ATXT
    assignDefaultTextures(*this, reader);

    layer.texture.formId = 0;
    for (int i = 0; i < 4; ++i)
        mTextures[i].base.formId = 0;

    while (reader.getSubRecordHeader())
    {
        const ESM4::SubRecordHeader& subHdr = reader.subRecordHeader();
        switch (subHdr.typeId)
        {
            case ESM::fourCC("DATA"):
            {
                reader.get(mLandFlags);
                break;
            }
            case ESM::fourCC("VNML"): // vertex normals, 33x33x(1+1+1) = 3267
            {
                reader.get(mVertNorm);
                mDataTypes |= LAND_VNML;
                break;
            }
            case ESM::fourCC("VHGT"): // vertex height gradient, 4+33x33+3 = 4+1089+3 = 1096
            {
                reader.get(mHeightMap);
                mDataTypes |= LAND_VHGT;
                break;
            }
            case ESM::fourCC("VCLR"): // vertex colours, 24bit RGB, 33x33x(1+1+1) = 3267
            {
                reader.get(mVertColr);
                mDataTypes |= LAND_VCLR;
                break;
            }
            case ESM::fourCC("BTXT"):
            {
                BTXT base;
                if (reader.getExact(base))
                {
                    if (base.quadrant >= 4)
                        throw std::runtime_error("base texture quadrant index error");

                    if (base.formId != 0)
                        reader.adjustFormId(base.formId);
                    mTextures[base.quadrant].base = base;
                }
                break;
            }
            case ESM::fourCC("ATXT"):
            {
                if (currentAddQuad != -1)
                {
                    // NOTE: sometimes there are no VTXT following an ATXT
                    layer.data.resize(1); // just one spot
                    layer.data.back().position = 0; // this corner
                    layer.data.back().opacity = 0.f; // transparent

                    if (layer.texture.layerIndex != mTextures[currentAddQuad].layers.size())
                        throw std::runtime_error("ESM4::LAND additional texture skipping layer");

                    mTextures[currentAddQuad].layers.push_back(layer);
                }

                reader.get(layer.texture);
                if (layer.texture.formId != 0)
                    reader.adjustFormId(layer.texture.formId);
                if (layer.texture.quadrant >= 4)
                    throw std::runtime_error("ESM4::LAND additional texture quadrant index error");

                currentAddQuad = layer.texture.quadrant;
                break;
            }
            case ESM::fourCC("VTXT"):
            {
                if (currentAddQuad == -1)
                    throw std::runtime_error("VTXT without ATXT found");

                const std::uint16_t count = reader.subRecordHeader().dataSize / sizeof(ESM4::Land::VTXT);
                if ((reader.subRecordHeader().dataSize % sizeof(ESM4::Land::VTXT)) != 0)
                    throw std::runtime_error("ESM4::LAND VTXT data size error");

                if (count)
                {
                    layer.data.resize(count);
                    for (ESM4::Land::VTXT& vtxt : layer.data)
                        reader.get(vtxt);
                }

                if (layer.texture.layerIndex != mTextures[currentAddQuad].layers.size())
                    throw std::runtime_error("ESM4::LAND additional texture skipping layer");

                mTextures[currentAddQuad].layers.push_back(layer);

                currentAddQuad = -1;
                layer.data.clear();
                break;
            }
            case ESM::fourCC("VTEX"): // only in Oblivion?
            {
                const std::uint16_t count = reader.subRecordHeader().dataSize / sizeof(ESM::FormId32);
                if ((reader.subRecordHeader().dataSize % sizeof(ESM::FormId32)) != 0)
                    throw std::runtime_error("ESM4::LAND VTEX data size error");

                if (count)
                {
                    mIds.resize(count);
                    for (ESM::FormId& id : mIds)
                        reader.getFormId(id);
                }
                break;
            }
            case ESM::fourCC("MPCD"): // FO4
                reader.skipSubRecordData();
                break;
            default:
                throw std::runtime_error("ESM4::LAND - Unknown subrecord " + ESM::printName(subHdr.typeId));
        }
    }

    if (currentAddQuad != -1)
    {
        // not sure if it happens here as well, if so just ignore
        Log(Debug::Verbose) << "ESM4::Land VTXT empty layer " << layer.texture.layerIndex << " quad "
                            << static_cast<unsigned>(layer.texture.quadrant);
    }
}
