/*
  Copyright (C) 2015, 2018, 2020 cc9cii

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
#ifndef ESM4_NAVM_H
#define ESM4_NAVM_H

#include <cstdint>
#include <vector>

#include "common.hpp" // CellGrid, Vertex

namespace ESM4
{
    class Reader;
    class Writer;

    struct NavMesh
    {
#pragma pack(push,1)
        struct Triangle
        {
            std::uint16_t vertexIndex0;
            std::uint16_t vertexIndex1;
            std::uint16_t vertexIndex2;
            std::uint16_t edge0;
            std::uint16_t edge1;
            std::uint16_t edge2;
            std::uint16_t coverMarker;
            std::uint16_t coverFlags;
        };

        struct ExtConnection
        {
            std::uint32_t unknown;
            FormId        navMesh;
            std::uint16_t triangleIndex;
        };

        struct DoorTriangle
        {
            std::uint16_t triangleIndex;
            std::uint32_t unknown;
            FormId        doorRef;
        };
#pragma pack(pop)

        struct NVNMstruct
        {
            std::uint32_t unknownNVER;
            std::uint32_t unknownLCTN;
            FormId        worldSpaceId;
            CellGrid cellGrid;
            std::vector<Vertex>        verticies;
            std::vector<Triangle>      triangles;
            std::vector<ExtConnection> extConns;
            std::vector<DoorTriangle>  doorTriangles;
            std::vector<std::uint16_t> coverTriangles;
            std::uint32_t divisor;
            float maxXDist;
            float maxYDist;
            float minX;
            float minY;
            float minZ;
            float maxX;
            float maxY;
            float maxZ;
            // there are divisor^2 segments, each segment is a vector of triangle indices
            std::vector<std::vector<std::uint16_t> >  triSegments;

            void load(ESM4::Reader& esm);
        };

        std::vector<NVNMstruct> mData; // Up to 4 skywind cells in one Morrowind cell

        FormId mFormId;       // from the header
        std::uint32_t mFlags; // from the header, see enum type RecordFlag for details

        NavMesh();
        virtual ~NavMesh();

        virtual void load(ESM4::Reader& reader);
        //virtual void save(ESM4::Writer& writer) const;

        void blank();
    };

}

#endif // ESM4_NAVM_H
