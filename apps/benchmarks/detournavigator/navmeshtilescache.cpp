#include <benchmark/benchmark.h>

#include <components/detournavigator/navmeshtilescache.hpp>
#include <components/detournavigator/stats.hpp>
#include <components/esm3/loadland.hpp>

#include <algorithm>
#include <iterator>
#include <random>

namespace
{
    using namespace DetourNavigator;

    struct Key
    {
        AgentBounds mAgentBounds;
        TilePosition mTilePosition;
        RecastMesh mRecastMesh;
    };

    struct Item
    {
        Key mKey;
        PreparedNavMeshData mValue;
    };

    osg::Vec2i generateVec2i(int max, auto& random)
    {
        std::uniform_int_distribution<int> distribution(0, max);
        return osg::Vec2i(distribution(random), distribution(random));
    }

    osg::Vec3f generateAgentHalfExtents(float min, float max, auto& random)
    {
        std::uniform_int_distribution<int> distribution(min, max);
        return osg::Vec3f(distribution(random), distribution(random), distribution(random));
    }

    void generateVertices(std::output_iterator<int> auto out, std::size_t number, auto& random)
    {
        std::uniform_real_distribution<float> distribution(0.0, 1.0);
        std::generate_n(out, 3 * (number - number % 3), [&] { return distribution(random); });
    }

    void generateIndices(std::output_iterator<int> auto out, int max, std::size_t number, auto& random)
    {
        std::uniform_int_distribution<int> distribution(0, max);
        std::generate_n(out, number - number % 3, [&] { return distribution(random); });
    }

    AreaType toAreaType(int index)
    {
        switch (index)
        {
            case 0:
                return AreaType_null;
            case 1:
                return AreaType_water;
            case 2:
                return AreaType_door;
            case 3:
                return AreaType_pathgrid;
            case 4:
                return AreaType_ground;
        }
        return AreaType_null;
    }

    AreaType generateAreaType(auto& random)
    {
        std::uniform_int_distribution<int> distribution(0, 4);
        return toAreaType(distribution(random));
    }

    void generateAreaTypes(std::output_iterator<AreaType> auto out, std::size_t triangles, auto& random)
    {
        std::generate_n(out, triangles, [&] { return generateAreaType(random); });
    }

    void generateWater(std::output_iterator<CellWater> auto out, std::size_t count, auto& random)
    {
        std::uniform_real_distribution<float> distribution(0.0, 1.0);
        std::generate_n(out, count, [&] {
            return CellWater{ generateVec2i(1000, random), Water{ ESM::Land::REAL_SIZE, distribution(random) } };
        });
    }

    Mesh generateMesh(std::size_t triangles, auto& random)
    {
        std::uniform_real_distribution<float> distribution(0.0, 1.0);
        std::vector<float> vertices;
        std::vector<int> indices;
        std::vector<AreaType> areaTypes;
        if (distribution(random) < 0.939)
        {
            generateVertices(std::back_inserter(vertices), static_cast<std::size_t>(triangles * 2.467), random);
            generateIndices(std::back_inserter(indices), static_cast<int>(vertices.size() / 3) - 1,
                static_cast<std::size_t>(vertices.size() * 1.279), random);
            generateAreaTypes(std::back_inserter(areaTypes), indices.size() / 3, random);
        }
        return Mesh(std::move(indices), std::move(vertices), std::move(areaTypes));
    }

    Heightfield generateHeightfield(auto& random)
    {
        std::uniform_real_distribution<float> distribution(0.0, 1.0);
        Heightfield result;
        result.mCellPosition = generateVec2i(1000, random);
        result.mCellSize = ESM::Land::REAL_SIZE;
        result.mMinHeight = distribution(random);
        result.mMaxHeight = result.mMinHeight + 1.0f;
        result.mLength = static_cast<std::uint8_t>(ESM::Land::LAND_SIZE);
        std::generate_n(
            std::back_inserter(result.mHeights), ESM::Land::LAND_NUM_VERTS, [&] { return distribution(random); });
        result.mOriginalSize = ESM::Land::LAND_SIZE;
        result.mMinX = 0;
        result.mMinY = 0;
        return result;
    }

    FlatHeightfield generateFlatHeightfield(auto& random)
    {
        std::uniform_real_distribution<float> distribution(0.0, 1.0);
        FlatHeightfield result;
        result.mCellPosition = generateVec2i(1000, random);
        result.mCellSize = ESM::Land::REAL_SIZE;
        result.mHeight = distribution(random);
        return result;
    }

    Key generateKey(std::size_t triangles, auto& random)
    {
        const CollisionShapeType agentShapeType = CollisionShapeType::Aabb;
        const osg::Vec3f agentHalfExtents = generateAgentHalfExtents(0.5, 1.5, random);
        const TilePosition tilePosition = generateVec2i(10000, random);
        const Version version{
            .mGeneration = std::uniform_int_distribution<std::size_t>(0, 100)(random),
            .mRevision = std::uniform_int_distribution<std::size_t>(0, 10000)(random),
        };
        Mesh mesh = generateMesh(triangles, random);
        std::vector<CellWater> water;
        generateWater(std::back_inserter(water), 1, random);
        RecastMesh recastMesh(version, std::move(mesh), std::move(water), { generateHeightfield(random) },
            { generateFlatHeightfield(random) }, {});
        return Key{ AgentBounds{ agentShapeType, agentHalfExtents }, tilePosition, std::move(recastMesh) };
    }

    constexpr std::size_t trianglesPerTile = 239;

    void generateKeys(std::output_iterator<Key> auto out, std::size_t count, auto& random)
    {
        std::generate_n(out, count, [&] { return generateKey(trianglesPerTile, random); });
    }

    void fillCache(std::output_iterator<Key> auto out, auto& random, NavMeshTilesCache& cache)
    {
        std::size_t size = cache.getStats().mNavMeshCacheSize;

        while (true)
        {
            Key key = generateKey(trianglesPerTile, random);
            cache.set(key.mAgentBounds, key.mTilePosition, key.mRecastMesh, std::make_unique<PreparedNavMeshData>());
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

        for ([[maybe_unused]] auto _ : state)
        {
            const auto& key = keys[n++ % keys.size()];
            auto result = cache.get(key.mAgentBounds, key.mTilePosition, key.mRecastMesh);
            benchmark::DoNotOptimize(result);
        }
    }

    void getFromFilledCache_1m_100hit(benchmark::State& state)
    {
        getFromFilledCache<1 * 1024 * 1024, 100>(state);
    }

    void getFromFilledCache_4m_100hit(benchmark::State& state)
    {
        getFromFilledCache<4 * 1024 * 1024, 100>(state);
    }

    void getFromFilledCache_16m_100hit(benchmark::State& state)
    {
        getFromFilledCache<16 * 1024 * 1024, 100>(state);
    }

    void getFromFilledCache_64m_100hit(benchmark::State& state)
    {
        getFromFilledCache<64 * 1024 * 1024, 100>(state);
    }

    void getFromFilledCache_1m_70hit(benchmark::State& state)
    {
        getFromFilledCache<1 * 1024 * 1024, 70>(state);
    }

    void getFromFilledCache_4m_70hit(benchmark::State& state)
    {
        getFromFilledCache<4 * 1024 * 1024, 70>(state);
    }

    void getFromFilledCache_16m_70hit(benchmark::State& state)
    {
        getFromFilledCache<16 * 1024 * 1024, 70>(state);
    }

    void getFromFilledCache_64m_70hit(benchmark::State& state)
    {
        getFromFilledCache<64 * 1024 * 1024, 70>(state);
    }

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
            auto result = cache.set(
                key.mAgentBounds, key.mTilePosition, key.mRecastMesh, std::make_unique<PreparedNavMeshData>());
            benchmark::DoNotOptimize(result);
        }
    }

    void setToBoundedNonEmptyCache_1m(benchmark::State& state)
    {
        setToBoundedNonEmptyCache<1 * 1024 * 1024>(state);
    }

    void setToBoundedNonEmptyCache_4m(benchmark::State& state)
    {
        setToBoundedNonEmptyCache<4 * 1024 * 1024>(state);
    }

    void setToBoundedNonEmptyCache_16m(benchmark::State& state)
    {
        setToBoundedNonEmptyCache<16 * 1024 * 1024>(state);
    }

    void setToBoundedNonEmptyCache_64m(benchmark::State& state)
    {
        setToBoundedNonEmptyCache<64 * 1024 * 1024>(state);
    }
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
