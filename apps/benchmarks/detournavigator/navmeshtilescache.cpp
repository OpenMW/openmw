#include <benchmark/benchmark.h>

#include <components/detournavigator/navmeshtilescache.hpp>
#include <components/esm/loadland.hpp>

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
    };

    struct Item
    {
        Key mKey;
        PreparedNavMeshData mValue;
    };

    template <typename Random>
    osg::Vec2i generateVec2i(int max, Random& random)
    {
        std::uniform_int_distribution<int> distribution(0, max);
        return osg::Vec2i(distribution(random), distribution(random));
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
        return toAreaType(distribution(random));
    }

    template <typename OutputIterator, typename Random>
    void generateAreaTypes(OutputIterator out, std::size_t triangles, Random& random)
    {
        std::generate_n(out, triangles, [&] { return generateAreaType(random); });
    }

    template <typename OutputIterator, typename Random>
    void generateWater(OutputIterator out, std::size_t count, Random& random)
    {
        std::uniform_real_distribution<float> distribution(0.0, 1.0);
        std::generate_n(out, count, [&] {
            return CellWater {generateVec2i(1000, random), Water {ESM::Land::REAL_SIZE, distribution(random)}};
        });
    }

    template <class Random>
    Mesh generateMesh(std::size_t triangles, Random& random)
    {
        std::uniform_real_distribution<float> distribution(0.0, 1.0);
        std::vector<float> vertices;
        std::vector<int> indices;
        std::vector<AreaType> areaTypes;
        if (distribution(random) < 0.939)
        {
            generateVertices(std::back_inserter(vertices), triangles * 2.467, random);
            generateIndices(std::back_inserter(indices), static_cast<int>(vertices.size() / 3) - 1, vertices.size() * 1.279, random);
            generateAreaTypes(std::back_inserter(areaTypes), indices.size() / 3, random);
        }
        return Mesh(std::move(indices), std::move(vertices), std::move(areaTypes));
    }

    template <class Random>
    Heightfield generateHeightfield(Random& random)
    {
        std::uniform_real_distribution<float> distribution(0.0, 1.0);
        Heightfield result;
        result.mCellPosition = generateVec2i(1000, random);
        result.mCellSize = ESM::Land::REAL_SIZE;
        result.mMinHeight = distribution(random);
        result.mMaxHeight = result.mMinHeight + 1.0;
        result.mLength = static_cast<std::uint8_t>(ESM::Land::LAND_SIZE);
        std::generate_n(std::back_inserter(result.mHeights), ESM::Land::LAND_NUM_VERTS, [&]
        {
            return distribution(random);
        });
        result.mOriginalSize = ESM::Land::LAND_SIZE;
        result.mMinX = 0;
        result.mMinY = 0;
        return result;
    }

    template <class Random>
    FlatHeightfield generateFlatHeightfield(Random& random)
    {
        std::uniform_real_distribution<float> distribution(0.0, 1.0);
        FlatHeightfield result;
        result.mCellPosition = generateVec2i(1000, random);
        result.mCellSize = ESM::Land::REAL_SIZE;
        result.mHeight = distribution(random);
        return result;
    }

    template <class Random>
    Key generateKey(std::size_t triangles, Random& random)
    {
        const osg::Vec3f agentHalfExtents = generateAgentHalfExtents(0.5, 1.5, random);
        const TilePosition tilePosition = generateVec2i(10000, random);
        const std::size_t generation = std::uniform_int_distribution<std::size_t>(0, 100)(random);
        const std::size_t revision = std::uniform_int_distribution<std::size_t>(0, 10000)(random);
        Mesh mesh = generateMesh(triangles, random);
        std::vector<CellWater> water;
        generateWater(std::back_inserter(water), 1, random);
        RecastMesh recastMesh(generation, revision, std::move(mesh), std::move(water),
                              {generateHeightfield(random)}, {generateFlatHeightfield(random)}, {});
        return Key {agentHalfExtents, tilePosition, std::move(recastMesh)};
    }

    constexpr std::size_t trianglesPerTile = 239;

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
            cache.set(key.mAgentHalfExtents, key.mTilePosition, key.mRecastMesh,
                      std::make_unique<PreparedNavMeshData>());
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
            const auto result = cache.get(key.mAgentHalfExtents, key.mTilePosition, key.mRecastMesh);
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
            const auto result = cache.set(key.mAgentHalfExtents, key.mTilePosition, key.mRecastMesh,
                                          std::make_unique<PreparedNavMeshData>());
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
