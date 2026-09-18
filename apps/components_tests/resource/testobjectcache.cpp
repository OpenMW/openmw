#include <components/resource/objectcache.hpp>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <osg/Object>

namespace Resource
{
    namespace
    {
        using namespace ::testing;

        TEST(ResourceGenericObjectCacheTest, getRefFromObjectCacheShouldReturnNullptrByDefault)
        {
            GenericObjectCache<int> cache;
            EXPECT_EQ(cache.getRefFromObjectCache(42), nullptr);
        }

        TEST(ResourceGenericObjectCacheTest, getRefFromObjectCacheOrNoneShouldReturnNulloptByDefault)
        {
            GenericObjectCache<int> cache;
            EXPECT_EQ(cache.getRefFromObjectCacheOrNone(42), std::nullopt);
        }

        struct Object : osg::Object
        {
            Object() = default;

            Object(const Object& other, const osg::CopyOp& copyOp = osg::CopyOp())
                : osg::Object(other, copyOp)
            {
            }

            META_Object(ResourceTest, Object)
        };

        TEST(ResourceGenericObjectCacheTest, shouldStoreValues)
        {
            GenericObjectCache<int> cache;
            const int key = 42;
            osg::ref_ptr<Object> value(new Object);
            cache.addEntryToObjectCache(key, value);
            EXPECT_EQ(cache.getRefFromObjectCache(key), value);
        }

        TEST(ResourceGenericObjectCacheTest, shouldStoreNullptrValues)
        {
            GenericObjectCache<int> cache;
            const int key = 42;
            cache.addEntryToObjectCache(key, nullptr);
            EXPECT_THAT(cache.getRefFromObjectCacheOrNone(key), Optional(nullptr));
        }

        TEST(ResourceGenericObjectCacheTest, updateShouldExtendLifetimeForItemsWithZeroTimestamp)
        {
            GenericObjectCache<int> cache;

            const int key = 42;
            osg::ref_ptr<Object> value(new Object);
            cache.addEntryToObjectCache(key, value, 0);
            value = nullptr;

            const double referenceTime = 1000;
            const double expiryDelay = 1;
            cache.update(referenceTime, expiryDelay);
            EXPECT_THAT(cache.getRefFromObjectCacheOrNone(key), Optional(_));
        }

        TEST(ResourceGenericObjectCacheTest, addEntryToObjectCacheShouldReplaceExistingItemByKey)
        {
            GenericObjectCache<int> cache;

            const int key = 42;
            osg::ref_ptr<Object> value1(new Object);
            osg::ref_ptr<Object> value2(new Object);
            cache.addEntryToObjectCache(key, value1);
            ASSERT_EQ(cache.getRefFromObjectCache(key), value1);
            cache.addEntryToObjectCache(key, value2);
            EXPECT_EQ(cache.getRefFromObjectCache(key), value2);
        }

        TEST(ResourceGenericObjectCacheTest, addEntryToObjectCacheShouldMarkLifetime)
        {
            GenericObjectCache<int> cache;

            const double referenceTime = 1;
            const double expiryDelay = 2;

            const int key = 42;
            cache.addEntryToObjectCache(key, nullptr, referenceTime + expiryDelay);

            cache.update(referenceTime, expiryDelay);
            ASSERT_THAT(cache.getRefFromObjectCacheOrNone(key), Optional(_));

            cache.update(referenceTime + expiryDelay, expiryDelay);
            ASSERT_THAT(cache.getRefFromObjectCacheOrNone(key), Optional(_));

            cache.update(referenceTime + 2 * expiryDelay, expiryDelay);
            EXPECT_EQ(cache.getRefFromObjectCacheOrNone(key), std::nullopt);
        }

        TEST(ResourceGenericObjectCacheTest, updateShouldRemoveExpiredItems)
        {
            GenericObjectCache<int> cache;

            const double referenceTime = 1;
            const double expiryDelay = 1;

            const int key = 42;
            osg::ref_ptr<Object> value(new Object);
            cache.addEntryToObjectCache(key, value);
            value = nullptr;

            cache.update(referenceTime, expiryDelay);
            ASSERT_THAT(cache.getRefFromObjectCacheOrNone(key), Optional(_));
            ASSERT_EQ(cache.getStats().mExpired, 0);

            cache.update(referenceTime + expiryDelay, expiryDelay);
            EXPECT_EQ(cache.getRefFromObjectCacheOrNone(key), std::nullopt);
            ASSERT_EQ(cache.getStats().mExpired, 1);
        }

        TEST(ResourceGenericObjectCacheTest, updateShouldKeepExternallyReferencedItems)
        {
            GenericObjectCache<int> cache;

            const double referenceTime = 1;
            const double expiryDelay = 1;

            const int key = 42;
            osg::ref_ptr<Object> value(new Object);
            cache.addEntryToObjectCache(key, value);

            cache.update(referenceTime, expiryDelay);
            ASSERT_THAT(cache.getRefFromObjectCacheOrNone(key), Optional(_));

            cache.update(referenceTime + expiryDelay, expiryDelay);
            EXPECT_THAT(cache.getRefFromObjectCacheOrNone(key), Optional(value));
        }

        TEST(ResourceGenericObjectCacheTest, updateShouldKeepNotExpiredItems)
        {
            GenericObjectCache<int> cache;

            const double referenceTime = 1;
            const double expiryDelay = 2;

            const int key = 42;
            osg::ref_ptr<Object> value(new Object);
            cache.addEntryToObjectCache(key, value);
            value = nullptr;

            cache.update(referenceTime + expiryDelay, expiryDelay);
            ASSERT_THAT(cache.getRefFromObjectCacheOrNone(key), Optional(_));

            cache.update(referenceTime + expiryDelay / 2, expiryDelay);
            EXPECT_THAT(cache.getRefFromObjectCacheOrNone(key), Optional(_));
        }

        TEST(ResourceGenericObjectCacheTest, updateShouldKeepNotExpiredNullptrItems)
        {
            GenericObjectCache<int> cache;

            const double referenceTime = 1;
            const double expiryDelay = 2;

            const int key = 42;
            cache.addEntryToObjectCache(key, nullptr);

            cache.update(referenceTime + expiryDelay, expiryDelay);
            ASSERT_THAT(cache.getRefFromObjectCacheOrNone(key), Optional(_));

            cache.update(referenceTime + expiryDelay / 2, expiryDelay);
            EXPECT_THAT(cache.getRefFromObjectCacheOrNone(key), Optional(_));
        }

        TEST(ResourceGenericObjectCacheTest, getRefFromObjectCacheOrNoneShouldNotExtendItemLifetime)
        {
            GenericObjectCache<int> cache;

            const double referenceTime = 1;
            const double expiryDelay = 2;

            const int key = 42;
            cache.addEntryToObjectCache(key, nullptr);

            cache.update(referenceTime, expiryDelay);
            ASSERT_THAT(cache.getRefFromObjectCacheOrNone(key), Optional(_));

            cache.update(referenceTime + expiryDelay / 2, expiryDelay);
            ASSERT_THAT(cache.getRefFromObjectCacheOrNone(key), Optional(_));

            cache.update(referenceTime + expiryDelay, expiryDelay);
            EXPECT_EQ(cache.getRefFromObjectCacheOrNone(key), std::nullopt);
        }

        TEST(ResourceGenericObjectCacheTest, lowerBoundShouldSupportHeterogeneousLookup)
        {
            GenericObjectCache<std::string> cache;
            cache.addEntryToObjectCache("a", nullptr);
            cache.addEntryToObjectCache("c", nullptr);
            EXPECT_THAT(cache.lowerBound(std::string_view("b")), Optional(Pair("c", _)));
        }

        TEST(ResourceGenericObjectCacheTest, shouldSupportRemovingItems)
        {
            GenericObjectCache<int> cache;
            const int key = 42;
            osg::ref_ptr<Object> value(new Object);
            cache.addEntryToObjectCache(key, value);
            ASSERT_EQ(cache.getRefFromObjectCache(key), value);
            cache.removeFromObjectCache(key);
            EXPECT_EQ(cache.getRefFromObjectCacheOrNone(key), std::nullopt);
        }

        TEST(ResourceGenericObjectCacheTest, clearShouldRemoveAllItems)
        {
            GenericObjectCache<int> cache;

            const int key1 = 42;
            const int key2 = 13;
            osg::ref_ptr<Object> value1(new Object);
            osg::ref_ptr<Object> value2(new Object);
            cache.addEntryToObjectCache(key1, value1);
            cache.addEntryToObjectCache(key2, value2);

            ASSERT_EQ(cache.getRefFromObjectCache(key1), value1);
            ASSERT_EQ(cache.getRefFromObjectCache(key2), value2);

            cache.clear();

            EXPECT_EQ(cache.getRefFromObjectCacheOrNone(key1), std::nullopt);
            EXPECT_EQ(cache.getRefFromObjectCacheOrNone(key2), std::nullopt);
        }

        TEST(ResourceGenericObjectCacheTest, callShouldIterateOverAllItems)
        {
            GenericObjectCache<int> cache;

            osg::ref_ptr<Object> value1(new Object);
            osg::ref_ptr<Object> value2(new Object);
            osg::ref_ptr<Object> value3(new Object);
            cache.addEntryToObjectCache(1, value1);
            cache.addEntryToObjectCache(2, value2);
            cache.addEntryToObjectCache(3, value3);

            std::vector<std::pair<int, osg::Object*>> actual;
            cache.call([&](int key, osg::Object* value) { actual.emplace_back(key, value); });

            EXPECT_THAT(actual, ElementsAre(Pair(1, value1.get()), Pair(2, value2.get()), Pair(3, value3.get())));
        }

        TEST(ResourceGenericObjectCacheTest, getStatsShouldReturnNumberOrAddedItems)
        {
            GenericObjectCache<int> cache;

            osg::ref_ptr<Object> value1(new Object);
            osg::ref_ptr<Object> value2(new Object);
            cache.addEntryToObjectCache(13, value1);
            cache.addEntryToObjectCache(42, value2);

            const CacheStats stats = cache.getStats();

            EXPECT_EQ(stats.mSize, 2);
        }

        TEST(ResourceGenericObjectCacheTest, getStatsShouldReturnNumberOrGetsAndHits)
        {
            GenericObjectCache<int> cache;

            {
                const CacheStats stats = cache.getStats();

                EXPECT_EQ(stats.mGet, 0);
                EXPECT_EQ(stats.mHit, 0);
            }

            osg::ref_ptr<Object> value(new Object);
            cache.addEntryToObjectCache(13, value);
            cache.getRefFromObjectCache(13);
            cache.getRefFromObjectCache(42);

            {
                const CacheStats stats = cache.getStats();

                EXPECT_EQ(stats.mGet, 2);
                EXPECT_EQ(stats.mHit, 1);
            }
        }

        TEST(ResourceGenericObjectCacheTest, lowerBoundShouldReturnFirstNotLessThatGivenKey)
        {
            GenericObjectCache<int> cache;

            osg::ref_ptr<Object> value1(new Object);
            osg::ref_ptr<Object> value2(new Object);
            osg::ref_ptr<Object> value3(new Object);
            cache.addEntryToObjectCache(1, value1);
            cache.addEntryToObjectCache(2, value2);
            cache.addEntryToObjectCache(4, value3);

            EXPECT_THAT(cache.lowerBound(3), Optional(Pair(4, value3)));
        }

        TEST(ResourceGenericObjectCacheTest, lowerBoundShouldReturnNulloptWhenKeyIsGreaterThanAnyOther)
        {
            GenericObjectCache<int> cache;

            osg::ref_ptr<Object> value1(new Object);
            osg::ref_ptr<Object> value2(new Object);
            osg::ref_ptr<Object> value3(new Object);
            cache.addEntryToObjectCache(1, value1);
            cache.addEntryToObjectCache(2, value2);
            cache.addEntryToObjectCache(3, value3);

            EXPECT_EQ(cache.lowerBound(4), std::nullopt);
        }

        TEST(ResourceGenericObjectCacheTest, addEntryToObjectCacheShouldSupportHeterogeneousLookup)
        {
            GenericObjectCache<std::string> cache;
            const std::string key = "key";
            osg::ref_ptr<Object> value(new Object);
            cache.addEntryToObjectCache(std::string_view("key"), value);
            EXPECT_EQ(cache.getRefFromObjectCache(key), value);
        }

        TEST(ResourceGenericObjectCacheTest, addEntryToObjectCacheShouldKeyMoving)
        {
            GenericObjectCache<std::string> cache;
            std::string key(128, 'a');
            osg::ref_ptr<Object> value(new Object);
            cache.addEntryToObjectCache(std::move(key), value);
            EXPECT_EQ(key, "");
            EXPECT_EQ(cache.getRefFromObjectCache(std::string(128, 'a')), value);
        }

        TEST(ResourceGenericObjectCacheTest, removeFromObjectCacheShouldSupportHeterogeneousLookup)
        {
            GenericObjectCache<std::string> cache;
            const std::string key = "key";
            osg::ref_ptr<Object> value(new Object);
            cache.addEntryToObjectCache(key, value);
            ASSERT_EQ(cache.getRefFromObjectCache(key), value);
            cache.removeFromObjectCache(std::string_view("key"));
            EXPECT_EQ(cache.getRefFromObjectCacheOrNone(key), std::nullopt);
        }

        TEST(ResourceGenericObjectCacheTest, getRefFromObjectCacheShouldSupportHeterogeneousLookup)
        {
            GenericObjectCache<std::string> cache;
            const std::string key = "key";
            osg::ref_ptr<Object> value(new Object);
            cache.addEntryToObjectCache(key, value);
            EXPECT_EQ(cache.getRefFromObjectCache(std::string_view("key")), value);
        }

        TEST(ResourceGenericObjectCacheTest, getRefFromObjectCacheOrNoneShouldSupportHeterogeneousLookup)
        {
            GenericObjectCache<std::string> cache;
            const std::string key = "key";
            osg::ref_ptr<Object> value(new Object);
            cache.addEntryToObjectCache(key, value);
            EXPECT_THAT(cache.getRefFromObjectCacheOrNone(std::string_view("key")), Optional(value));
        }

        TEST(ResourceGenericObjectCacheTest, checkInObjectCacheShouldSupportHeterogeneousLookup)
        {
            GenericObjectCache<std::string> cache;
            const std::string key = "key";
            osg::ref_ptr<Object> value(new Object);
            cache.addEntryToObjectCache(key, value);
            EXPECT_TRUE(cache.checkInObjectCache(std::string_view("key"), 0));
        }
    }
}
