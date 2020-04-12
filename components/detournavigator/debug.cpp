#include "debug.hpp"
#include "exceptions.hpp"
#include "recastmesh.hpp"

#include <DetourNavMesh.h>

#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>

namespace DetourNavigator
{
    void writeToFile(const RecastMesh& recastMesh, const std::string& pathPrefix, const std::string& revision)
    {
        const auto path = pathPrefix + "recastmesh" + revision + ".obj";
        boost::filesystem::ofstream file(boost::filesystem::path(path), std::ios::out);
        if (!file.is_open())
            throw NavigatorException("Open file failed: " + path);
        file.exceptions(std::ios::failbit | std::ios::badbit);
        file.precision(std::numeric_limits<float>::max_exponent10);
        std::size_t count = 0;
        for (auto v : recastMesh.getVertices())
        {
            if (count % 3 == 0)
            {
                if (count != 0)
                    file << '\n';
                file << 'v';
            }
            file << ' ' << v;
            ++count;
        }
        file << '\n';
        count = 0;
        for (auto v : recastMesh.getIndices())
        {
            if (count % 3 == 0)
            {
                if (count != 0)
                    file << '\n';
                file << 'f';
            }
            file << ' ' << (v + 1);
            ++count;
        }
        file << '\n';
    }

    void writeToFile(const dtNavMesh& navMesh, const std::string& pathPrefix, const std::string& revision)
    {
        const int navMeshSetMagic = 'M' << 24 | 'S' << 16 | 'E' << 8 | 'T'; //'MSET';
        const int navMeshSetVersion = 1;

        struct NavMeshSetHeader
        {
            int magic;
            int version;
            int numTiles;
            dtNavMeshParams params;
        };

        struct NavMeshTileHeader
        {
            dtTileRef tileRef;
            int dataSize;
        };

        const auto path = pathPrefix + "all_tiles_navmesh" + revision + ".bin";
        boost::filesystem::ofstream file(boost::filesystem::path(path), std::ios::out | std::ios::binary);
        if (!file.is_open())
            throw NavigatorException("Open file failed: " + path);
        file.exceptions(std::ios::failbit | std::ios::badbit);

        NavMeshSetHeader header;
        header.magic = navMeshSetMagic;
        header.version = navMeshSetVersion;
        header.numTiles = 0;
        for (int i = 0; i < navMesh.getMaxTiles(); ++i)
        {
            const auto tile = navMesh.getTile(i);
            if (!tile || !tile->header || !tile->dataSize)
                continue;
            header.numTiles++;
        }
        header.params = *navMesh.getParams();

        using const_char_ptr = const char*;
        file.write(const_char_ptr(&header), sizeof(header));

        for (int i = 0; i < navMesh.getMaxTiles(); ++i)
        {
            const auto tile = navMesh.getTile(i);
            if (!tile || !tile->header || !tile->dataSize)
                continue;

            NavMeshTileHeader tileHeader;
            tileHeader.tileRef = navMesh.getTileRef(tile);
            tileHeader.dataSize = tile->dataSize;

            file.write(const_char_ptr(&tileHeader), sizeof(tileHeader));
            file.write(const_char_ptr(tile->data), tile->dataSize);
        }
    }
}
