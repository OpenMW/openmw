#include <components/resource/multiobjectcache.hpp>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <memory>
#include <string>

namespace Resource
{
    namespace
    {
        using namespace ::testing;

        // A value type that derives from nothing, which is the point of the value
        // type parameter; BulletShapeInstance is the real user.
        using Cache = MultiObjectCache<std::shared_ptr<const std::string>>;

        constexpr VFS::Path::NormalizedView path("meshes/a.nif");
        constexpr VFS::Path::NormalizedView otherPath("meshes/b.nif");

        TEST(ResourceMultiObjectCacheTest, takeShouldReturnNullForAbsentKey)
        {
            Cache cache;
            EXPECT_EQ(cache.takeFromObjectCache(path), nullptr);
        }

        TEST(ResourceMultiObjectCacheTest, takeShouldRemoveTheEntry)
        {
            Cache cache;
            const auto value = std::make_shared<const std::string>("value");
            cache.addEntryToObjectCache(path, value);

            EXPECT_EQ(cache.takeFromObjectCache(path), value);
            // "Non reusable": taking is a move out, not a read.
            EXPECT_EQ(cache.takeFromObjectCache(path), nullptr);
        }

        TEST(ResourceMultiObjectCacheTest, shouldHoldMoreThanOneValuePerKey)
        {
            Cache cache;
            cache.addEntryToObjectCache(path, std::make_shared<const std::string>("first"));
            cache.addEntryToObjectCache(path, std::make_shared<const std::string>("second"));

            EXPECT_NE(cache.takeFromObjectCache(path), nullptr);
            EXPECT_NE(cache.takeFromObjectCache(path), nullptr);
            EXPECT_EQ(cache.takeFromObjectCache(path), nullptr);
        }

        TEST(ResourceMultiObjectCacheTest, shouldRefuseNullValues)
        {
            Cache cache;
            cache.addEntryToObjectCache(path, nullptr);
            EXPECT_EQ(cache.getStats().mSize, 0u);
        }

        TEST(ResourceMultiObjectCacheTest, removeUnreferencedShouldKeepValuesHeldElsewhere)
        {
            Cache cache;
            const auto held = std::make_shared<const std::string>("held");
            cache.addEntryToObjectCache(path, held);
            cache.addEntryToObjectCache(otherPath, std::make_shared<const std::string>("dropped"));

            cache.removeUnreferencedObjectsInCache();

            EXPECT_EQ(cache.takeFromObjectCache(path), held);
            EXPECT_EQ(cache.takeFromObjectCache(otherPath), nullptr);
        }

        TEST(ResourceMultiObjectCacheTest, clearShouldRemoveEverything)
        {
            Cache cache;
            const auto held = std::make_shared<const std::string>("held");
            cache.addEntryToObjectCache(path, held);

            cache.clear();

            EXPECT_EQ(cache.getStats().mSize, 0u);
            EXPECT_EQ(cache.takeFromObjectCache(path), nullptr);
        }

        TEST(ResourceMultiObjectCacheTest, statsShouldCountGetsHitsAndExpiries)
        {
            Cache cache;
            cache.addEntryToObjectCache(path, std::make_shared<const std::string>("value"));
            cache.addEntryToObjectCache(otherPath, std::make_shared<const std::string>("dropped"));

            cache.takeFromObjectCache(path);
            cache.takeFromObjectCache(path);
            cache.removeUnreferencedObjectsInCache();

            const CacheStats stats = cache.getStats();
            EXPECT_EQ(stats.mGet, 2u);
            EXPECT_EQ(stats.mHit, 1u);
            EXPECT_EQ(stats.mExpired, 1u);
        }
    }
}
