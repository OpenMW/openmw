/*
  Copyright (C) 2015-2016, 2018, 2020 cc9cii

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
#ifndef ESM4_NAVI_H
#define ESM4_NAVI_H

#include <cstdint>
#include <map>
#include <vector>

#include <components/esm/defs.hpp>
#include <components/esm/formid.hpp>
#include <components/esm/gameformat.hpp>

#include "cellgrid.hpp"
#include "vertex.hpp"

namespace ESM4
{
    class Reader;
    class Writer;

    struct Navigation
    {
#pragma pack(push, 1)
        struct DoorRef
        {
            std::uint32_t crc; // CRC hash of "PathingDoor"
            ESM::FormId32 formId;
        };

        struct Triangle
        {
            std::uint16_t vertexIndex0;
            std::uint16_t vertexIndex1;
            std::uint16_t vertexIndex2;
        };
#pragma pack(pop)

        struct IslandInfo
        {
            float minX;
            float minY;
            float minZ;
            float maxX;
            float maxY;
            float maxZ;
            std::vector<Triangle> triangles;
            std::vector<Vertex> verticies;

            void loadBounds(ESM4::Reader& reader);
            void load(ESM4::Reader& reader); // TES5 and FO4
        };

        enum Flags // NVMI flags
        {
            FLG_InitiallyDisabled = 0x00000010, // FO3 and FNV
            FLG_Island = 0x00000020,
            FLG_Modified = 0x00000000, // Unused?
            FLG_Unmodified = 0x00000040 // TES5 and FO4
        };

        struct NavMeshInfo
        {
            ESM::FormId formId;
            std::uint32_t flags;
            // center point of the navmesh
            float x;
            float y;
            float z;
            float preferredPercent; // percent of triangles flagged "preferred"
            std::vector<ESM::FormId> formIdMerged;
            std::vector<ESM::FormId> formIdPrefMerged;
            std::vector<DoorRef> linkedDoors;
            std::vector<IslandInfo> islandInfo;
            std::uint32_t crc; // CRC hash of "PathingCell"
            ESM::FormId worldSpaceId;
            CellGrid cellGrid;

            void loadFO3(ESM4::Reader& reader); // FO3 and FNV
            void loadTES5(ESM4::Reader& reader); // TES5 and FO4
        };

        ESM::GameFormat mGameFormat = ESM::GameFormat::Unknown; // assumed per-file from context clues

        std::string mEditorId;

        std::vector<NavMeshInfo> mNavMeshInfo;

        std::vector<std::vector<ESM::FormId>> mPreferredPaths; // N x N navmesh paths between nodes

        std::map<ESM::FormId, std::uint32_t> mPathIndexMap; // node -> row and column in mPreferredPaths

        void load(ESM4::Reader& reader);
        // void save(ESM4::Writer& writer) const;

        // void blank();
        static constexpr ESM::RecNameInts sRecordId = ESM::RecNameInts::REC_NAVI4;
    };
}

#endif // ESM4_NAVI_H
