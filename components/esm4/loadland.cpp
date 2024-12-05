/*
  Copyright (C) 2015-2016, 2018, 2020-2021 cc9cii

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
#include "loadland.hpp"

#include <cstdint>
#include <stdexcept>

#include <components/debug/debuglog.hpp>

#include "reader.hpp"
// #include "writer.hpp"

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

    // std::map<FormId, int> uniqueTextures; // FIXME: for temp testing only

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
#if 0
                reader.get(mHeightMap.heightOffset);
                reader.get(mHeightMap.gradientData);
                reader.get(mHeightMap.unknown1);
                reader.get(mHeightMap.unknown2);
#endif
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
#if 0
                    std::cout << "Base Texture formid: 0x"
                        << std::hex << mTextures[base.quadrant].base.formId
                        << ", quad " << std::dec << (int)base.quadrant << std::endl;
#endif
                }
                break;
            }
            case ESM::fourCC("ATXT"):
            {
                if (currentAddQuad != -1)
                {
                    // FIXME: sometimes there are no VTXT following an ATXT?  Just add a dummy one for now
                    Log(Debug::Verbose) << "ESM4::Land VTXT empty layer " << layer.texture.layerIndex;
                    mTextures[currentAddQuad].layers.push_back(layer);
                }
                reader.get(layer.texture);
                reader.adjustFormId(layer.texture.formId);
                if (layer.texture.quadrant >= 4)
                    throw std::runtime_error("additional texture quadrant index error");
#if 0
                FormId txt = layer.texture.formId;
                std::map<FormId, int>::iterator lb = uniqueTextures.lower_bound(txt);
                if (lb != uniqueTextures.end() && !(uniqueTextures.key_comp()(txt, lb->first)))
                {
                    lb->second += 1;
                }
                else
                    uniqueTextures.insert(lb, std::make_pair(txt, 1));
#endif
#if 0
                std::cout << "Additional Texture formId: 0x"
                    << std::hex << layer.texture.formId
                    << ", quad " << std::dec << (int)layer.texture.quadrant << std::endl;
                std::cout << "Additional Texture layer: "
                    << std::dec << (int)layer.texture.layerIndex << std::endl;
#endif
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
                    {
                        reader.get(*it);
                        // FIXME: debug only
                        // std::cout << "pos: " << std::dec << (int)(*it).position << std::endl;
                    }
                }
                mTextures[currentAddQuad].layers.push_back(layer);

                // Assumed that the layers are added in the correct sequence
                // FIXME: Knights.esp doesn't seem to observe this - investigate more
                // assert(layer.texture.layerIndex == mTextures[currentAddQuad].layers.size()-1
                //&& "additional texture layer index error");

                currentAddQuad = -1;
                layer.data.clear();
                // FIXME: debug only
                // std::cout << "VTXT: count " << std::dec << count << std::endl;
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

    if (currentAddQuad != -1)
    {
        // FIXME: not sure if it happens here as well
        Log(Debug::Verbose) << "ESM4::Land VTXT empty layer " << layer.texture.layerIndex << " quad "
                            << static_cast<unsigned>(layer.texture.quadrant);
        mTextures[currentAddQuad].layers.push_back(layer);
    }

    bool missing = false;
    for (int i = 0; i < 4; ++i)
    {
        if (mTextures[i].base.formId == 0)
        {
            // std::cout << "ESM4::LAND " << ESM4::formIdToString(mFormId) << " missing base, quad " << i << std::endl;
            // std::cout << "layers " << mTextures[i].layers.size() << std::endl;
            //  NOTE: can't set the default here since FO3/FONV may have different defaults
            // mTextures[i].base.formId = 0x000008C0; // TerrainHDDirt01.dds
            missing = true;
        }
        // else
        //{
        //     std::cout << "ESM4::LAND " << ESM4::formIdToString(mFormId) << " base, quad " << i << std::endl;
        //     std::cout << "layers " << mTextures[i].layers.size() << std::endl;
        // }
    }
    // at least one of the quadrants do not have a base texture, return without setting the flag
    if (!missing)
        mDataTypes |= LAND_VTEX;
}

// void ESM4::Land::save(ESM4::Writer& writer) const
//{
// }

// void ESM4::Land::blank()
//{
// }
