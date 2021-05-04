#include <benchmark/benchmark.h>

#include <components/detournavigator/navmeshtilescache.hpp>

#include <algorithm>
#include <random>
#include <iostream>

namespace
{
    using namespace DetourNavigator;

    struct Key
    {
        osg::Vec3f mAgentHalfExtents;
        TilePosition mTilePosition;
        RecastMesh mRecastMesh;
        std::vector<OffMeshConnection> mOffMeshConnections;
    };

    struct Item
    {
        Key mKey;
        NavMeshData mValue;
    };

    template <typename Random>
    TilePosition generateTilePosition(int max, Random& random)
    {
        std::uniform_int_distribution<int> distribution(0, max);
        return TilePosition(distribution(random), distribution(random));
    }

    template <typename Random>
    osg::Vec3f generateAgentHalfExtents(float min, float max, Random& random)
    {
        std::uniform_int_distribution<int> distribution(min, max);
        return osg::Vec3f(distribution(random), distribution(random), distribution(random));
    }

    template <typename OutputIterator, typename Random>
    void generateVertices(OutputIterator out, std::size_t number, Random& random)
    {
        std::uniform_real_distribution<float> distribution(0.0, 1.0);
        std::generate_n(out, 3 * (number - number % 3), [&] { return distribution(random); });
    }

    template <typename OutputIterator, typename Random>
    void generateIndices(OutputIterator out, int max, std::size_t number, Random& random)
    {
        std::uniform_int_distribution<int> distribution(0, max);
        std::generate_n(out, number - number % 3, [&] { return distribution(random); });
    }

    AreaType toAreaType(int index)
    {
        switch (index)
        {
            case 0: return AreaType_null;
            case 1: return AreaType_water;
            case 2: return AreaType_door;
            case 3: return AreaType_pathgrid;
            case 4: return AreaType_ground;
        }
        return AreaType_null;
    }

    template <typename Random>
    AreaType generateAreaType(Random& random)
    {
        std::uniform_int_distribution<int> distribution(0, 4);
        return toAreaType(distribution(random));;
    }

    template <typename OutputIterator, typename Random>
    void generateAreaTypes(OutputIterator out, std::size_t triangles, Random& random)
    {
        std::generate_n(out, triangles, [&] { return generateAreaType(random); });
    }

    template <typename OutputIterator, typename Random>
    void generateWater(OutputIterator out, std::size_t count, Random& random)
    {
        std::uniform_real_distribution<btScalar> distribution(0.0, 1.0);
        std::generate_n(out, count, [&] {
            const btVector3 shift(distribution(random), distribution(random), distribution(random));
            return RecastMesh::Water {1, btTransform(btMatrix3x3::getIdentity(), shift)};
        });
    }

    template <typename OutputIterator, typename Random>
    void generateOffMeshConnection(OutputIterator out, std::size_t count, Random& random)
    {
        std::uniform_real_distribution<btScalar> distribution(0.0, 1.0);
        std::generate_n(out, count, [&] {
            const osg::Vec3f start(distribution(random), distribution(random), distribution(random));
            const osg::Vec3f end(distribution(random), distribution(random), distribution(random));
            return OffMeshConnection {start, end, generateAreaType(random)};
        });
    }

    template <class Random>
    Key generateKey(std::size_t triangles, Random& random)
    {
        const osg::Vec3f agentHalfExtents = generateAgentHalfExtents(0.5, 1.5, random);
        const TilePosition tilePosition = generateTilePosition(10000, random);
        const std::size_t generation = std::uniform_int_distribution<std::size_t>(0, 100)(random);
        const std::size_t revision = std::uniform_int_distribution<std::size_t>(0, 10000)(random);
        std::vector<float> vertices;
        generateVertices(std::back_inserter(vertices), triangles * 1.98, random);
        std::vector<int> indices;
        generateIndices(std::back_inserter(indices), static_cast<int>(vertices.size() / 3) - 1, vertices.size() * 1.53, random);
        std::vector<AreaType> areaTypes;
        generateAreaTypes(std::back_inserter(areaTypes), indices.size() / 3, random);
        std::vector<RecastMesh::Water> water;
        generateWater(std::back_inserter(water), 2, random);
        RecastMesh recastMesh(generation, revision, std::move(indices), std::move(vertices),
                              std::move(areaTypes), std::move(water));
        std::vector<OffMeshConnection> offMeshConnections;
        generateOffMeshConnection(std::back_inserter(offMeshConnections), 300, random);
        return Key {agentHalfExtents, tilePosition, std::move(recastMesh), std::move(offMeshConnections)};
    }

    constexpr std::size_t trianglesPerTile = 310;

    template <typename OutputIterator, typename Random>
    void generateKeys(OutputIterator out, std::size_t count, Random& random)
    {
        std::generate_n(out, count, [&] { return generateKey(trianglesPerTile, random); });
    }

    template <typename OutputIterator, typename Random>
    void fillCache(OutputIterator out, Random& random, NavMeshTilesCache& cache)
    {
        std::size_t size = cache.getStats().mNavMeshCacheSize;

        while (true)
        {
            Key key = generateKey(trianglesPerTile, random);
            cache.set(key.mAgentHalfExtents, key.mTilePosition, key.mRecastMesh, key.mOffMeshConnections, NavMeshData());
            *out++ = std::move(key);
            const std::size_t newSize = cache.getStats().mNavMeshCacheSize;
            if (size >= newSize)
                break;
            size = newSize;
        }
    }

    template <std::size_t maxCacheSize, int hitPercentage>
    void getFromFilledCache(benchmark::State& state)
    {
        NavMeshTilesCache cache(maxCacheSize);
        std::minstd_rand random;
        std::vector<Key> keys;
        fillCache(std::back_inserter(keys), random, cache);
        generateKeys(std::back_inserter(keys), keys.size() * (100 - hitPercentage) / 100, random);
        std::size_t n = 0;

        while (state.KeepRunning())
        {
            const auto& key = keys[n++ % keys.size()];
            const auto result = cache.get(key.mAgentHalfExtents, key.mTilePosition, key.mRecastMesh, key.mOffMeshConnections);
            benchmark::DoNotOptimize(result);
        }
    }

    constexpr auto getFromFilledCache_1m_100hit = getFromFilledCache<1 * 1024 * 1024, 100>;
    constexpr auto getFromFilledCache_4m_100hit = getFromFilledCache<4 * 1024 * 1024, 100>;
    constexpr auto getFromFilledCache_16m_100hit = getFromFilledCache<16 * 1024 * 1024, 100>;
    constexpr auto getFromFilledCache_64m_100hit = getFromFilledCache<64 * 1024 * 1024, 100>;
    constexpr auto getFromFilledCache_1m_70hit = getFromFilledCache<1 * 1024 * 1024, 70>;
    constexpr auto getFromFilledCache_4m_70hit = getFromFilledCache<4 * 1024 * 1024, 70>;
    constexpr auto getFromFilledCache_16m_70hit = getFromFilledCache<16 * 1024 * 1024, 70>;
    constexpr auto getFromFilledCache_64m_70hit = getFromFilledCache<64 * 1024 * 1024, 70>;

    template <std::size_t maxCacheSize>
    void setToBoundedNonEmptyCache(benchmark::State& state)
    {
        NavMeshTilesCache cache(maxCacheSize);
        std::minstd_rand random;
        std::vector<Key> keys;
        fillCache(std::back_inserter(keys), random, cache);
        generateKeys(std::back_inserter(keys), keys.size() * 2, random);
        std::reverse(keys.begin(), keys.end());
        std::size_t n = 0;

        while (state.KeepRunning())
        {
            const auto& key = keys[n++ % keys.size()];
            const auto result = cache.set(key.mAgentHalfExtents, key.mTilePosition, key.mRecastMesh, key.mOffMeshConnections, NavMeshData());
            benchmark::DoNotOptimize(result);
        }
    }

    constexpr auto setToBoundedNonEmptyCache_1m = setToBoundedNonEmptyCache<1 * 1024 * 1024>;
    constexpr auto setToBoundedNonEmptyCache_4m = setToBoundedNonEmptyCache<4 * 1024 * 1024>;
    constexpr auto setToBoundedNonEmptyCache_16m = setToBoundedNonEmptyCache<16 * 1024 * 1024>;
    constexpr auto setToBoundedNonEmptyCache_64m = setToBoundedNonEmptyCache<64 * 1024 * 1024>;
} // namespace

BENCHMARK(getFromFilledCache_1m_100hit);
BENCHMARK(getFromFilledCache_4m_100hit);
BENCHMARK(getFromFilledCache_16m_100hit);
BENCHMARK(getFromFilledCache_64m_100hit);
BENCHMARK(getFromFilledCache_1m_70hit);
BENCHMARK(getFromFilledCache_4m_70hit);
BENCHMARK(getFromFilledCache_16m_70hit);
BENCHMARK(getFromFilledCache_64m_70hit);
BENCHMARK(setToBoundedNonEmptyCache_1m);
BENCHMARK(setToBoundedNonEmptyCache_4m);
BENCHMARK(setToBoundedNonEmptyCache_16m);
BENCHMARK(setToBoundedNonEmptyCache_64m);

BENCHMARK_MAIN();
