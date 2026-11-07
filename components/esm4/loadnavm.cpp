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
#include "loadnavm.hpp"

#include <cstring>
#include <stdexcept>

#include <components/debug/debuglog.hpp>

#include "reader.hpp"
//#include "writer.hpp"

void ESM4::NavMesh::NVNMstruct::load(ESM4::Reader& reader)
{
    // std::cout << "start: divisor " << std::dec << divisor << ", segments " << triSegments.size() << //std::endl;
    //"this 0x" << this << std::endl; // FIXME

    std::uint32_t count;

    reader.get(version);
    reader.get(crc);

    readPathingCell(reader, worldSpaceId, cellGrid);
    // Exterior grid:
    //   ^
    // Y |                   X Y Index
    //     |                 0,0 0
    //   1 |23               0,1 1
    //   0 |01               1,0 2
    //     +---              1,1 3
    //      01 ->
    //         X
    //
    // e.g. Dagonfel X:13,14,15,16 Y:43,44,45,46 (Morrowind X:7 Y:22)
    //
    // Skywind:   -4,-3 -2,-1 0,1 2,3 4,5 6,7
    // Morrowind: -2    -1    0   1   2   3
    //
    // Formula seems to be floor(Skywind coord / 2) <cmath>

    reader.get(count); // numVerticies
    if (count)
    {
        verticies.resize(count);
        for (std::vector<Vertex>::iterator it = verticies.begin(); it != verticies.end(); ++it)
        {
            reader.get(*it);
// FIXME: debugging only
#if 0
            //if (reader.hdr().record.id == 0x2004ecc) // FIXME
            std::cout << "nvnm vert " << (*it).x << ", " << (*it).y << ", " << (*it).z << std::endl;
#endif
        }
    }

    reader.get(count); // numTriangles;
    if (count)
    {
        triangles.resize(count);
        for (std::vector<Triangle>::iterator it = triangles.begin(); it != triangles.end(); ++it)
        {
            reader.get(*it);
        }
    }

    reader.get(count); // numExtConn;
    if (count)
    {
        extConns.resize(count);
        for (std::vector<ExtConnection>::iterator it = extConns.begin(); it != extConns.end(); ++it)
        {
            reader.get(*it);
// FIXME: debugging only
#if 0
            std::cout << "nvnm ext 0x" << std::hex << (*it).navMesh << std::endl;
#endif
        }
    }

    reader.get(count); // numDoorTriangles;
    if (count)
    {
        doorTriangles.resize(count);
        for (std::vector<DoorTriangle>::iterator it = doorTriangles.begin(); it != doorTriangles.end(); ++it)
        {
            reader.get(*it);
        }
    }

    reader.get(count); // numCoverTriangles;
    if (count)
    {
        coverTriangles.resize(count);
        for (std::vector<std::uint16_t>::iterator it = coverTriangles.begin(); it != coverTriangles.end(); ++it)
        {
            reader.get(*it);
        }
    }

    // abs((maxX - minX) / divisor) = Max X Distance
    reader.get(divisor); // FIXME: latest over-writes old

    reader.get(maxXDist); // FIXME: update with formula
    reader.get(maxYDist);
    reader.get(minX); // FIXME: use std::min
    reader.get(minY);
    reader.get(minZ);
    reader.get(maxX);
    reader.get(maxY);
    reader.get(maxZ);

    // FIXME: should check remaining size here
    // there are divisor^2 segments, each segment is a vector of triangle indices
    for (unsigned int i = 0; i < divisor * divisor; ++i)
    {
        reader.get(count); // NOTE: count may be zero

        std::vector<std::uint16_t> indices;
        indices.resize(count);
        for (std::vector<std::uint16_t>::iterator it = indices.begin(); it != indices.end(); ++it)
        {
            reader.get(*it);
        }
        triSegments.push_back(std::move(indices));
    }
    if (triSegments.size() != divisor * divisor)
        throw std::runtime_error("Triangle segments size is not the square of divisor");
#if 0
    if (triSegments.size() != divisor*divisor)
        std::cout << "divisor " << std::dec << divisor << ", segments " << triSegments.size() << //std::endl;
        "this 0x" << this << std::endl;
#endif
}

void ESM4::NavMesh::load(ESM4::Reader& reader)
{
    mId = reader.getFormIdFromHeader();
    mFlags = reader.hdr().record.flags;
    mGameFormat = reader.gameFormat();

    // std::cout << "NavMesh 0x" << std::hex << this << std::endl; // FIXME

// FIXME: debugging only
#if 0
    std::string padding;
    padding.insert(0, reader.stackSize()*2, ' ');
    std::cout << padding << "NAVM flags 0x" << std::hex << reader.hdr().record.flags << std::endl;
    std::cout << padding << "NAVM id 0x" << std::hex << reader.hdr().record.id << std::endl;
#endif
    while (reader.getSubRecordHeader())
    {
        const ESM4::SubRecordHeader& subHdr = reader.subRecordHeader();
        switch (subHdr.typeId)
        {
            case ESM::fourCC("NVNM"):
            {
                // FO4 updates the format
                if (mGameFormat == ESM::GameFormat::FO4)
                {
                    reader.skipSubRecordData();
                    break;
                }

                NVNMstruct nvnm;
                nvnm.load(reader);
                mData.push_back(std::move(nvnm)); // FIXME try swap
                break;
            }
            case ESM::fourCC("ONAM"):
            case ESM::fourCC("PNAM"):
            case ESM::fourCC("NNAM"):
            case ESM::fourCC("NVER"): // FO3
            case ESM::fourCC("DATA"): // FO3
            case ESM::fourCC("NVVX"): // FO3
            case ESM::fourCC("NVTR"): // FO3
            case ESM::fourCC("NVCA"): // FO3
            case ESM::fourCC("NVDP"): // FO3
            case ESM::fourCC("NVGD"): // FO3
            case ESM::fourCC("NVEX"): // FO3
            case ESM::fourCC("EDID"): // FO3
            case ESM::fourCC("MNAM"): // FO4
                reader.skipSubRecordData();
                break;
            default:
                throw std::runtime_error("ESM4::NAVM::load - Unknown subrecord " + ESM::printName(subHdr.typeId));
        }
    }
    // std::cout << "num nvnm " << std::dec << mData.size() << std::endl; // FIXME
}

// void ESM4::NavMesh::save(ESM4::Writer& writer) const
//{
// }

void ESM4::NavMesh::blank() {}
