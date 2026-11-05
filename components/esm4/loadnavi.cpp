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
#include "loadnavi.hpp"

#include <stdexcept>

#include "reader.hpp"
//#include "writer.hpp"

void ESM4::Navigation::IslandInfo::loadBounds(ESM4::Reader& reader)
{
    reader.get(minX);
    reader.get(minY);
    reader.get(minZ);
    reader.get(maxX);
    reader.get(maxY);
    reader.get(maxZ);
}

void ESM4::Navigation::IslandInfo::load(ESM4::Reader& reader)
{
    loadBounds(reader);

    std::uint32_t count;
    reader.get(count); // countTriangle;
    if (count)
    {
        triangles.resize(count);
        // std::cout << "NVMI island triangles " << std::dec << count << std::endl; // FIXME
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
            std::string padding;
            padding.insert(0, reader.stackSize()*2, ' ');
            std::cout << padding << "NVMI vert " << std::dec << (*it).x << ", " << (*it).y << ", " << (*it).z << std::endl;
#endif
        }
    }
}

void ESM4::Navigation::NavMeshInfo::loadFO3(ESM4::Reader& reader)
{
    reader.get(flags);
    reader.getFormId(formId);
    reader.getFormId(worldSpaceId); // the cell or worldspace
    Grid grid;
    reader.get(grid.y); // NOTE: reverse order
    reader.get(grid.x);
    cellGrid = grid;
    reader.get(x);
    reader.get(y);
    reader.get(z);

    if (flags & FLG_Island)
    {
        IslandInfo island;
        island.loadBounds(reader);

        std::uint16_t vertexCount;
        std::uint16_t triangleCount;
        reader.get(vertexCount);
        reader.get(triangleCount);
        island.verticies.resize(vertexCount);
        for (ESM4::Vertex& vertex : island.verticies)
            reader.get(vertex);
        island.triangles.resize(triangleCount);
        for (Triangle& triangle : island.triangles)
            reader.get(triangle);

        islandInfo.push_back(std::move(island));
    }

    reader.get(preferredPercent);
}

void ESM4::Navigation::NavMeshInfo::loadTES5(ESM4::Reader& reader)
{
    std::uint32_t count;

    reader.getFormId(formId);
    reader.get(flags);
    reader.get(x);
    reader.get(y);
    reader.get(z);

// FIXME: for debugging only
#if 0
    std::string padding;
    if (flags & FLG_Unmodified)
        padding.insert(0, 4, '.');
    else
        padding.insert(0, 2, '-');

    padding.insert(0, reader.stackSize()*2, ' ');
    std::cout << padding << "NVMI formId: 0x" << std::hex << formId << std::endl;
    std::cout << padding << "NVMI flags: " << std::hex << flags << std::endl;
    std::cout << padding << "NVMI center: " << std::dec << x << ", " << y << ", " << z << std::endl;
#endif

    reader.get(preferredPercent);

    reader.get(count); // countMerged;
    if (count)
    {
        // std::cout << "NVMI countMerged " << std::dec << count << std::endl;
        formIdMerged.resize(count);
        for (ESM::FormId& value : formIdMerged)
            reader.getFormId(value);
    }

    reader.get(count); // countPrefMerged;
    if (count)
    {
        // std::cout << "NVMI countPrefMerged " << std::dec << count << std::endl;
        formIdPrefMerged.resize(count);
        for (ESM::FormId& value : formIdPrefMerged)
            reader.getFormId(value);
    }

    reader.get(count); // countLinkedDoors;
    if (count)
    {
        // std::cout << "NVMI countLinkedDoors " << std::dec << count << std::endl;
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
        Navigation::IslandInfo island2;
        island2.load(reader);
        islandInfo.push_back(std::move(island2)); // Maybe don't use a vector for just one entry?
    }
    // else if (flags == FLG_Island) // FIXME: debug only
    //   std::cerr << "nvmi no island but has 0x20 flag" << std::endl;

    reader.get(crc);
    readPathingCell(reader, worldSpaceId, cellGrid);
}

// NVPP is a matrix of preferred paths between N "node" navmeshes
// (TES5 10, FO4 14, fewer in some mods):
//
//   N * N | N * N x { count | navmesh formid's } | N | N x { node formid | index }
//
// Entry i * N + j is the path of navmeshes from node i to node j, and a node's path to itself is just that node.
// Index is a node's row and column in the matrix. The index values are not sequential, and the same nodes can
// have different indices in different files (e.g. Update.esm).
//
// Skyrim.esm's nodes:
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
    // mFormId = reader.hdr().record.getFormId();
    // mFlags  = reader.hdr().record.flags;
    mGameFormat = reader.gameFormat();

    while (reader.getSubRecordHeader())
    {
        const ESM4::SubRecordHeader& subHdr = reader.subRecordHeader();
        switch (subHdr.typeId)
        {
            case ESM::fourCC("EDID"): // seems to be unused?
            {
                if (!reader.getZString(mEditorId))
                    throw std::runtime_error("NAVI EDID data read error");
                break;
            }
            case ESM::fourCC("NVPP"):
            {
                std::uint32_t count;
                reader.get(count);
                mPreferredPaths.resize(count);
                for (std::vector<ESM::FormId>& path : mPreferredPaths)
                {
                    reader.get(count);
                    path.resize(count);
                    for (ESM::FormId& navMesh : path)
                        reader.getFormId(navMesh);
                }

                reader.get(count);
                for (std::uint32_t i = 0; i < count; ++i)
                {
                    ESM::FormId node;
                    std::uint32_t index;
                    reader.getFormId(node);
                    reader.get(index);
                    mPathIndexMap.emplace(node, index);
                }
                break;
            }
            case ESM::fourCC("NVER"):
            {
                std::uint32_t version; // always the same? (0x0c)
                reader.get(version); // TODO: store this or use it for merging?
                // std::cout << "NAVI version " << std::dec << version << std::endl;
                break;
            }
            case ESM::fourCC("NVMI"): // FO3+
            {
                switch (mGameFormat)
                {
                    case ESM::GameFormat::FO3:
                    case ESM::GameFormat::FNV:
                        mNavMeshInfo.emplace_back().loadFO3(reader);
                        break;
                    case ESM::GameFormat::TES5:
                    case ESM::GameFormat::FO4:
                        mNavMeshInfo.emplace_back().loadTES5(reader);
                        break;
                    default:
                        reader.skipSubRecordData();
                        break;
                }
                break;
            }
            case ESM::fourCC("NVSI"): // TES5 deleted navmeshes, from Dawnguard onwards
            case ESM::fourCC("NVCI"): // FO3 and FNV navmesh connections
            {
                reader.skipSubRecordData(); // FIXME: not decoded yet
                break;
            }
            default:
            {
                throw std::runtime_error("ESM4::NAVI::load - Unknown subrecord " + ESM::printName(subHdr.typeId));
            }
        }
    }
}

// void ESM4::Navigation::save(ESM4::Writer& writer) const
//{
// }

// void ESM4::Navigation::blank()
//{
// }
