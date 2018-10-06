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
#include "navi.hpp"

#include <cassert>
#include <stdexcept>

#include <iostream> // FIXME: debugging only

#include "reader.hpp"
//#include "writer.hpp"

#ifdef NDEBUG // FIXME: debuggigng only
#undef NDEBUG
#endif

ESM4::Navigation::Navigation()
{
    mEditorId.clear();
}

ESM4::Navigation::~Navigation()
{
}

void ESM4::Navigation::IslandInfo::load(ESM4::Reader& reader)
{
    reader.get(minX);
    reader.get(minY);
    reader.get(minZ);
    reader.get(maxX);
    reader.get(maxY);
    reader.get(maxZ);

    std::uint32_t count;
    reader.get(count); // countTriangle;
    if (count)
    {
        triangles.resize(count);
        //std::cout << "NVMI island triangles " << std::dec << count << std::endl; // FIXME
        for (std::vector<Navigation::Triangle>::iterator it = triangles.begin(); it != triangles.end(); ++it)
        {
            reader.get(*it);
        }
    }

    reader.get(count); // countVertex;
    if (count)
    {
        verticies.resize(count);
        for (std::vector<ESM4::Vertex>::iterator it = verticies.begin(); it != verticies.end(); ++it)
        {
            reader.get(*it);
// FIXME: debugging only
#if 0
            std::string padding = "";
            padding.insert(0, reader.stackSize()*2, ' ');
            std::cout << padding << "NVMI vert " << std::dec << (*it).x << ", " << (*it).y << ", " << (*it).z << std::endl;
#endif
        }
    }
}

void ESM4::Navigation::NavMeshInfo::load(ESM4::Reader& reader)
{
    std::uint32_t count;

    reader.get(formId);
    reader.get(flags);
    reader.get(x);
    reader.get(y);
    reader.get(z);

// FIXME: for debugging only
#if 0
    std::string padding = "";
    if (flags == ESM4::FLG_Modified)
        padding.insert(0, 2, '-');
    else if (flags == ESM4::FLG_Unmodified)
        padding.insert(0, 4, '.');

    padding.insert(0, reader.stackSize()*2, ' ');
    std::cout << padding << "NVMI formId: 0x" << std::hex << formId << std::endl;
    std::cout << padding << "NVMI flags: " << std::hex << flags << std::endl;
    std::cout << padding << "NVMI center: " << std::dec << x << ", " << y << ", " << z << std::endl;
#endif

    reader.get(flagPrefMerges);

    reader.get(count); // countMerged;
    if (count)
    {
        //std::cout << "NVMI countMerged " << std::dec << count << std::endl;
        formIdMerged.resize(count);
        for (std::vector<FormId>::iterator it = formIdMerged.begin(); it != formIdMerged.end(); ++it)
        {
            reader.get(*it);
        }
    }

    reader.get(count); // countPrefMerged;
    if (count)
    {
        //std::cout << "NVMI countPrefMerged " << std::dec << count << std::endl;
        formIdPrefMerged.resize(count);
        for (std::vector<FormId>::iterator it = formIdPrefMerged.begin(); it != formIdPrefMerged.end(); ++it)
        {
            reader.get(*it);
        }
    }

    reader.get(count); // countLinkedDoors;
    if (count)
    {
        //std::cout << "NVMI countLinkedDoors " << std::dec << count << std::endl;
        linkedDoors.resize(count);
        for (std::vector<DoorRef>::iterator it = linkedDoors.begin(); it != linkedDoors.end(); ++it)
        {
            reader.get(*it);
        }
    }

    unsigned char island;
    reader.get(island);
    if (island)
    {
        Navigation::IslandInfo island;
        island.load(reader);
        islandInfo.push_back(island); // Maybe don't use a vector for just one entry?
    }
    else if (flags == FLG_Island) // FIXME: debug only
        std::cerr << "nvmi no island but has 0x20 flag" << std::endl;

    reader.get(locationMarker);

    reader.get(worldSpaceId);
    //FLG_Tamriel    = 0x0000003c, // grid info follows, possibly Tamriel?
    //FLG_Morrowind  = 0x01380000, // grid info follows, probably Skywind
    if (worldSpaceId == 0x0000003c || worldSpaceId == 0x01380000)
    {
        reader.get(cellGrid.grid.y); // NOTE: reverse order
        reader.get(cellGrid.grid.x);
// FIXME: debugging only
#if 0
    std::string padding = "";
    padding.insert(0, reader.stackSize()*2, ' ');
    if (worldSpaceId == ESM4::FLG_Morrowind)
        std::cout << padding << "NVMI MW: X " << std::dec << cellGrid.grid.x << ", Y " << cellGrid.grid.y << std::endl;
    else
        std::cout << padding << "NVMI SR: X " << std::dec << cellGrid.grid.x << ", Y " << cellGrid.grid.y << std::endl;
#endif
    }
    else
    {
        reader.get(cellGrid.cellId);

#if 0
        if (worldSpaceId == 0) // interior
            std::cout << "NVMI Interior: cellId " << std::hex << cellGrid.cellId << std::endl;
        else
            std::cout << "NVMI FormID: cellId " << std::hex << cellGrid.cellId << std::endl;
#endif
    }
}

// NVPP data seems to be organised this way (total is 0x64 = 100)
//
//  (0) total | 0x1 | formid (index 0) | count | formid's
//  (1)                                | count | formid's
//  (2)                                | count | formid's
//  (3)                                | count | formid's
//  (4)                                | count | formid's
//  (5)                                | count | formid's
//  (6)                                | count | formid's
//  (7)                                | count | formid's
//  (8)                                | count | formid's
//  (9)                                | count | formid's
// (10)       | 0x1 | formid (index 1) | count | formid's
// (11)                                | count | formid's
// (12)                                | count | formid's
// (13)                                | count | formid's
// (14)                                | count | formid's
// (15)                                | count | formid's
//  ...
//
// (88)                                | count | formid's
// (89)                                | count | formid's
//
// Here the pattern changes (final count is 0xa = 10)
//
// (90)       | 0x1 | formid (index 9) | count | formid | index
// (91)                                        | formid | index
// (92)                                        | formid | index
// (93)                                        | formid | index
// (94)                                        | formid | index
// (95)                                        | formid | index
// (96)                                        | formid | index
// (97)                                        | formid | index
// (98)                                        | formid | index
// (99)                                        | formid | index
//
// Note that the index values are not sequential, i.e. the first index value
// (i.e. row 90) for Update.esm is 2.
//
// Also note that there's no list of formid's following the final node (index 9)
//
// The same 10 formids seem to be used for the indicies, but not necessarily
// with the same index value (but only Update.esm differs?)
//
// formid   cellid   X   Y Editor ID                   other formids in same X,Y    S U D D
// -------- ------ --- --- --------------------------- ---------------------------- - - - -
// 00079bbf 9639     5  -4 WhiterunExterior17          00079bc3                     0 6 0 0
// 0010377b 8ed5     6  24 DawnstarWesternMineExterior                              1 1 1 1
// 000a3f44 9577   -22   2 RoriksteadEdge                                           2 9 2 2
// 00100f4b 8ea2    26  25 WinterholdExterior01        00100f4a, 00100f49           3 3 3 3
// 00103120 bc8e    42 -22 (near Riften)                                            4 2 4 4
// 00105e9a 929d   -18  24 SolitudeExterior03                                       5 0 5 5
// 001030cb 7178   -40   1 SalviusFarmExterior01       (east of Markarth)           6 8 6 6
// 00098776 980b     4 -19 HelgenExterior              000cce3d                     7 5 7 7
// 000e88cc 93de    -9  14 (near Morthal)              0010519e, 0010519d, 000e88d2 8 7 8 8
// 000b87df b51d    33   5 WindhelmAttackStart05                                    9 4 9 9
//
void ESM4::Navigation::load(ESM4::Reader& reader)
{
    //mFormId = reader.hdr().record.id;
    //mFlags  = reader.hdr().record.flags;
    std::uint32_t esmVer = reader.esmVersion();
    bool isFONV = esmVer == ESM4::VER_132 || esmVer == ESM4::VER_133 || esmVer == ESM4::VER_134;

    while (reader.getSubRecordHeader())
    {
        const ESM4::SubRecordHeader& subHdr = reader.subRecordHeader();
        switch (subHdr.typeId)
        {
            case ESM4::SUB_EDID: // seems to be unused?
            {
                if (!reader.getZString(mEditorId))
                    throw std::runtime_error ("NAVI EDID data read error");
                break;
            }
            case ESM4::SUB_NVPP:
            {
                std::uint32_t total;
                std::uint32_t count;
                reader.get(total);
                if (!total)
                {
                    reader.get(count); // throw away
                    break;
                }

                total -= 10; // HACK
                std::uint32_t node;
                for (std::uint32_t i = 0; i < total; ++i)
                {
                    std::vector<FormId> preferredPaths;
                    reader.get(count);
                    if (count == 1)
                    {
                        reader.get(node);
                        reader.get(count);
                    }
                    if (count)
                    {
                        preferredPaths.resize(count);
                        for (std::vector<FormId>::iterator it = preferredPaths.begin();
                                it != preferredPaths.end(); ++it)
                        {
                            reader.get(*it);
                        }
                    }
                    mPreferredPaths.push_back(std::make_pair(node, preferredPaths));
#if 0
                    std::cout << "node " << std::hex << node // FIXME: debugging only
                        << ", count " << count << ", i " << std::dec << i << std::endl;
#endif
                }
                reader.get(count);
                assert(count == 1 && "expected separator");

                reader.get(node); // HACK
                std::vector<FormId> preferredPaths;
                mPreferredPaths.push_back(std::make_pair(node, preferredPaths)); // empty
#if 0
                std::cout << "node " << std::hex << node // FIXME: debugging only
                        << ", count " << 0 << std::endl;
#endif

                reader.get(count); // HACK
                assert(count == 10 && "expected 0xa");
                std::uint32_t index;
                for (std::uint32_t i = 0; i < count; ++i)
                {
                    reader.get(node);
                    reader.get(index);
#if 0
                    std::cout << "node " << std::hex << node // FIXME: debugging only
                        << ", index " << index << ", i " << std::dec << total+i << std::endl;
#endif
                    std::pair<std::map<FormId, std::uint32_t>::iterator, bool> res =
                        mPathIndexMap.insert(std::make_pair(node, index));
                    // FIXME: this throws if more than one file is being loaded
                    //if (!res.second)
                        //throw std::runtime_error ("node already exists in the preferred path index map");
                }
                break;
            }
            case ESM4::SUB_NVER:
            {
                std::uint32_t version; // always the same? (0x0c)
                reader.get(version); // TODO: store this or use it for merging?
                //std::cout << "NAVI version " << std::dec << version << std::endl;
                break;
            }
            case ESM4::SUB_NVMI: // multiple
            {
                if (esmVer == ESM4::VER_094 || esmVer == ESM4::VER_170 || isFONV)
                {
                    reader.skipSubRecordData(); // FIXME: FO3/FONV have different form of NavMeshInfo
                    break;
                }

                //std::cout << "\nNVMI start" << std::endl;
                NavMeshInfo nvmi;
                nvmi.load(reader);
                mNavMeshInfo.push_back (nvmi);
                break;
            }
            case ESM4::SUB_NVSI: // from Dawnguard onwards
            case ESM4::SUB_NVCI: // FO3
            {
                reader.skipSubRecordData(); // FIXME:
                break;
            }
            default:
            {
                throw std::runtime_error("ESM4::NAVI::load - Unknown subrecord " + ESM4::printName(subHdr.typeId));
            }
        }
    }
}

//void ESM4::Navigation::save(ESM4::Writer& writer) const
//{
//}

//void ESM4::Navigation::blank()
//{
//}
