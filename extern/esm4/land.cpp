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
#include "land.hpp"

#include <cassert>
#include <stdexcept>

#include <iostream> // FIXME: debug only

#include "reader.hpp"
//#include "writer.hpp"

#ifdef NDEBUG // FIXME: debuggigng only
#undef NDEBUG
#endif

ESM4::Land::Land() : mFormId(0), mFlags(0), mLandFlags(0), mDataTypes(0)
{
    for (int i = 0; i < 4; ++i)
        mTextures[i].base.formId = 0;
}

ESM4::Land::~Land()
{
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
    mFormId = reader.hdr().record.id;
    reader.adjustFormId(mFormId);
    mFlags  = reader.hdr().record.flags;

    TxtLayer layer;
    std::int8_t currentAddQuad = -1; // for VTXT following ATXT

    while (reader.getSubRecordHeader())
    {
        const ESM4::SubRecordHeader& subHdr = reader.subRecordHeader();
        switch (subHdr.typeId)
        {
            case ESM4::SUB_DATA:
            {
                reader.get(mLandFlags);
                break;
            }
            case ESM4::SUB_VNML: // vertex normals, 33x33x(1+1+1) = 3267
            {
                reader.get(mVertNorm);
                mDataTypes |= LAND_VNML;
                break;
            }
            case ESM4::SUB_VHGT: // vertex height gradient, 4+33x33+3 = 4+1089+3 = 1096
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
            case ESM4::SUB_VCLR: // vertex colours, 24bit RGB, 33x33x(1+1+1) = 3267
            {
                reader.get(mVertColr);
                mDataTypes |= LAND_VCLR;
                break;
            }
            case ESM4::SUA_BTXT:
            {
                BTXT base;
                if (reader.get(base))
                {
                    assert(base.quadrant < 4 && base.quadrant >= 0 && "base texture quadrant index error");

                    reader.adjustFormId(base.formId);
                    mTextures[base.quadrant].base = base;  // FIXME: any way to avoid double-copying?
#if 0
                    std::cout << "Base Texture formid: 0x"
                        << std::hex << mTextures[base.quadrant].base.formId
                        << ", quad " << std::dec << (int)base.quadrant << std::endl;
#endif
                }
                break;
            }
            case ESM4::SUB_ATXT:
            {
                if (currentAddQuad != -1)
                {
                    // FIXME: sometimes there are no VTXT following an ATXT?  Just add a dummy one for now
                    //std::cout << "ESM4::Land VTXT empty layer " << (int)layer.additional.layer << std::endl;
                    mTextures[currentAddQuad].layers.push_back(layer);
                }
                reader.get(layer.additional);
                reader.adjustFormId(layer.additional.formId);
                assert(layer.additional.quadrant < 4 && layer.additional.quadrant >= 0
                       && "additional texture quadrant index error");
#if 0
                std::cout << "Additional Texture formId: 0x"
                    << std::hex << layer.additional.formId
                    << ", quad " << std::dec << (int)layer.additional.quadrant << std::endl;
                std::cout << "Additional Texture layer: "
                    << std::dec << (int)layer.additional.layer << std::endl;
#endif
                currentAddQuad = layer.additional.quadrant;
                break;
            }
            case ESM4::SUB_VTXT:
            {
                assert(currentAddQuad != -1 && "VTXT without ATXT found");

                int count = (int)reader.subRecordHeader().dataSize / sizeof(ESM4::Land::VTXT);
                int remainder = reader.subRecordHeader().dataSize % sizeof(ESM4::Land::VTXT);
                assert(remainder == 0 && "ESM4::LAND VTXT data size error");

                if (count)
                {
                    layer.data.resize(count);
                    std::vector<ESM4::Land::VTXT>::iterator it = layer.data.begin();
                    for (;it != layer.data.end(); ++it)
                    {
                        reader.get(*it);
                        // FIXME: debug only
                        //std::cout << "pos: " << std::dec << (int)(*it).position << std::endl;
                    }
                }
                mTextures[currentAddQuad].layers.push_back(layer);

                // Assumed that the layers are added in the correct sequence
                // FIXME: Knights.esp doesn't seem to observe this - investigate more
                //assert(layer.additional.layer == mTextures[currentAddQuad].layers.size()-1
                        //&& "additional texture layer index error");

                currentAddQuad = -1;
                // FIXME: debug only
                //std::cout << "VTXT: count " << std::dec << count << std::endl;
                break;
            }
            case ESM4::SUB_VTEX: // only in Oblivion?
            {
                int count = (int)reader.subRecordHeader().dataSize / sizeof(FormId);
                int remainder = reader.subRecordHeader().dataSize % sizeof(FormId);
                assert(remainder == 0 && "ESM4::LAND VTEX data size error");

                if (count)
                {
                    mIds.resize(count);
                    for (std::vector<FormId>::iterator it = mIds.begin(); it != mIds.end(); ++it)
                    {
                        reader.getFormId(*it);
                        // FIXME: debug only
                        //std::cout << "VTEX: " << std::hex << *it << std::endl;
                    }
                }
                break;
            }
            default:
                throw std::runtime_error("ESM4::LAND::load - Unknown subrecord " + ESM4::printName(subHdr.typeId));
        }
    }

    bool missing = false;
    for (int i = 0; i < 4; ++i)
    {
        if (mTextures[i].base.formId == 0)
        {
            //std::cout << "ESM::LAND " << ESM4::formIdToString(mFormId) << " missing base, quad " << i << std::endl;
            missing = true;
        }
    }
    // at least one of the quadrants do not have a base texture, return without setting the flag
    if (!missing)
        mDataTypes |= LAND_VTEX;
}

//void ESM4::Land::save(ESM4::Writer& writer) const
//{
//}

//void ESM4::Land::blank()
//{
//}
