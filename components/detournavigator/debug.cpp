#include "debug.hpp"

#define OPENMW_WRITE_TO_FILE
#define OPENMW_WRITE_OBJ

#ifdef OPENMW_WRITE_OBJ
#include "exceptions.hpp"

#include <fstream>
#endif

#ifdef OPENMW_WRITE_TO_FILE
#include "exceptions.hpp"
#include "recastmesh.hpp"

#include <DetourNavMesh.h>

#include <fstream>
#endif

#include <iomanip>
#include <limits>

namespace
{
#ifdef OPENMW_WRITE_TO_FILE
    static const int NAVMESHSET_MAGIC = 'M' << 24 | 'S' << 16 | 'E' << 8 | 'T'; //'MSET';
    static const int NAVMESHSET_VERSION = 1;

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
#endif
}

namespace DetourNavigator
{
// Use to dump scene to load from recastnavigation demo tool
#ifdef OPENMW_WRITE_OBJ
    void writeObj(const std::vector<float>& vertices, const std::vector<int>& indices)
    {
        const auto path = std::string("scene.") + std::to_string(std::time(nullptr)) + ".obj";
        std::ofstream file(path);
        if (!file.is_open())
            throw NavigatorException("Open file failed: " + path);
        file.exceptions(std::ios::failbit | std::ios::badbit);
        file.precision(std::numeric_limits<float>::max_exponent10);
        std::size_t count = 0;
        for (auto v : vertices)
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
        for (auto v : indices)
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
#endif

#ifdef OPENMW_WRITE_TO_FILE
    void writeToFile(const RecastMesh& recastMesh, const std::string& revision)
    {
        const auto path = "recastmesh." + revision + ".obj";
        std::ofstream file(path);
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

    void writeToFile(const dtNavMesh& navMesh, const std::string& revision)
    {
        const auto path = "navmesh." + revision + ".bin";
        std::ofstream file(path, std::ios::binary);
        if (!file.is_open())
            throw NavigatorException("Open file failed: " + path);
        file.exceptions(std::ios::failbit | std::ios::badbit);

        NavMeshSetHeader header;
        header.magic = NAVMESHSET_MAGIC;
        header.version = NAVMESHSET_VERSION;
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
#endif
}
