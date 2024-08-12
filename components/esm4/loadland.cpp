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

#include <cstdint>
#include <cassert>
#include <stdexcept>
#include <iostream>

#include <components/debug/debuglog.hpp>

#include "reader.hpp"
// #include "writer.hpp"

namespace
{
    std::uint32_t getDefaultTexture(bool isTES4, bool isFONV, bool isTES5)
    {
        // WARN: guessed for FO3/FONV (might be Dirt02)
        if (isTES4)
            return 0x000008C0; // TerrainHDDirt01.dds (LTEX)
        else if (isFONV)
            return 0x00000A0D; // Landscape\Dirt01.dds (TXST 0x00004453)
        else if (isTES5)
            return 0x00000C16; // Landscape\Dirt02.dds (TXST 0x00000C0F)
        else // FO3
            return 0x00000A0D; // Landscape\Dirt01.dds (prob. same as FONV)
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

    std::uint32_t esmVer = reader.esmVersion();
    bool isTES4 = (esmVer == ESM::VER_080 || esmVer == ESM::VER_100);
    bool isFONV = (esmVer == ESM::VER_132 || esmVer == ESM::VER_133 || esmVer == ESM::VER_134);
    bool isTES5 = (esmVer == ESM::VER_094 || esmVer == ESM::VER_170); // WARN: FO3 is also VER_094
    // WARN: below workaround assumes the data directory path has "Fallout" somewhere
    //if (esmVer == ESM::VER_094 && reader.getContext().filename.find("allout") != std::string::npos)
    //    isTES5 = false; // FIXME: terrible hack

    mDataTypes = 0;
    mCell = reader.currCell();
    TxtLayer layer;
    std::int8_t currentAddQuad = -1; // for VTXT following ATXT

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

                    reader.adjustFormId(base.formId);
                    mTextures[base.quadrant].base = std::move(base);
                }
                break;
            }
            case ESM::fourCC("ATXT"):
            {
                if (currentAddQuad != -1)
                {
                    // NOTE: sometimes there are no VTXT following an ATXT
                    //Log(Debug::Verbose) << "ESM4::Land VTXT empty layer " << layer.texture.layerIndex
                        //<< " FormId " << ESM::FormId::toString(mFormId) << std::endl;
                    if (!layer.texture.formId)
                      layer.texture.formId = getDefaultTexture(isTES4, isFONV, isTES5);

                    layer.data.resize(1);            // just one spot
                    layer.data.back().position = 0;  // this corner
                    layer.data.back().opacity = 0.f; // transparent

                    assert(layer.texture.layerIndex == mTextures[currentAddQuad].layers.size()
                            && "additional texture skipping layer");

                    mTextures[currentAddQuad].layers.push_back(layer);
                }

                reader.get(layer.texture);
                reader.adjustFormId(layer.texture.formId);
                if (layer.texture.quadrant >= 4)
                    throw std::runtime_error("additional texture quadrant index error");

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
                    std::vector<ESM4::Land::VTXT>::iterator it = layer.data.begin();
                    for (; it != layer.data.end(); ++it)
                        reader.get(*it);
                }

                assert(layer.texture.layerIndex == mTextures[currentAddQuad].layers.size()
                        && "additional texture skipping layer");

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
                throw std::runtime_error("ESM4::LAND::load - Unknown subrecord " + ESM::printName(subHdr.typeId));
        }
    }

    //if (mCell.toUint32() == 0x00005e1f)
        //std::cout << "vilverin exterior" << std::endl;

    if (currentAddQuad != -1)
    {
        // not sure if it happens here as well, if so just ignore
        Log(Debug::Verbose) << "ESM4::Land VTXT empty layer " << layer.texture.layerIndex << " quad "
                            << static_cast<unsigned>(layer.texture.quadrant);
    }

    for (int i = 0; i < 4; ++i)
    {
        // just use some defaults
        if (mTextures[i].base.formId == 0)
            mTextures[i].base.formId = getDefaultTexture(isTES4, isFONV, isTES5);
    }
}

// void ESM4::Land::save(ESM4::Writer& writer) const
//{
// }

// void ESM4::Land::blank()
//{
// }
