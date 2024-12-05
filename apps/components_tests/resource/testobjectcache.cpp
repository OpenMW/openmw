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
            osg::ref_ptr<GenericObjectCache<int>> cache(new GenericObjectCache<int>);
            EXPECT_EQ(cache->getRefFromObjectCache(42), nullptr);
        }

        TEST(ResourceGenericObjectCacheTest, getRefFromObjectCacheOrNoneShouldReturnNulloptByDefault)
        {
            osg::ref_ptr<GenericObjectCache<int>> cache(new GenericObjectCache<int>);
            EXPECT_EQ(cache->getRefFromObjectCacheOrNone(42), std::nullopt);
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
            osg::ref_ptr<GenericObjectCache<int>> cache(new GenericObjectCache<int>);
            const int key = 42;
            osg::ref_ptr<Object> value(new Object);
            cache->addEntryToObjectCache(key, value);
            EXPECT_EQ(cache->getRefFromObjectCache(key), value);
        }

        TEST(ResourceGenericObjectCacheTest, shouldStoreNullptrValues)
        {
            osg::ref_ptr<GenericObjectCache<int>> cache(new GenericObjectCache<int>);
            const int key = 42;
            cache->addEntryToObjectCache(key, nullptr);
            EXPECT_THAT(cache->getRefFromObjectCacheOrNone(key), Optional(nullptr));
        }

        TEST(ResourceGenericObjectCacheTest, updateShouldExtendLifetimeForItemsWithZeroTimestamp)
        {
            osg::ref_ptr<GenericObjectCache<int>> cache(new GenericObjectCache<int>);

            const int key = 42;
            osg::ref_ptr<Object> value(new Object);
            cache->addEntryToObjectCache(key, value, 0);
            value = nullptr;

            const double referenceTime = 1000;
            const double expiryDelay = 1;
            cache->update(referenceTime, expiryDelay);
            EXPECT_THAT(cache->getRefFromObjectCacheOrNone(key), Optional(_));
        }

        TEST(ResourceGenericObjectCacheTest, addEntryToObjectCacheShouldReplaceExistingItemByKey)
        {
            osg::ref_ptr<GenericObjectCache<int>> cache(new GenericObjectCache<int>);

            const int key = 42;
            osg::ref_ptr<Object> value1(new Object);
            osg::ref_ptr<Object> value2(new Object);
            cache->addEntryToObjectCache(key, value1);
            ASSERT_EQ(cache->getRefFromObjectCache(key), value1);
            cache->addEntryToObjectCache(key, value2);
            EXPECT_EQ(cache->getRefFromObjectCache(key), value2);
        }

        TEST(ResourceGenericObjectCacheTest, addEntryToObjectCacheShouldMarkLifetime)
        {
            osg::ref_ptr<GenericObjectCache<int>> cache(new GenericObjectCache<int>);

            const double referenceTime = 1;
            const double expiryDelay = 2;

            const int key = 42;
            cache->addEntryToObjectCache(key, nullptr, referenceTime + expiryDelay);

            cache->update(referenceTime, expiryDelay);
            ASSERT_THAT(cache->getRefFromObjectCacheOrNone(key), Optional(_));

            cache->update(referenceTime + expiryDelay, expiryDelay);
            ASSERT_THAT(cache->getRefFromObjectCacheOrNone(key), Optional(_));

            cache->update(referenceTime + 2 * expiryDelay, expiryDelay);
            EXPECT_EQ(cache->getRefFromObjectCacheOrNone(key), std::nullopt);
        }

        TEST(ResourceGenericObjectCacheTest, updateShouldRemoveExpiredItems)
        {
            osg::ref_ptr<GenericObjectCache<int>> cache(new GenericObjectCache<int>);

            const double referenceTime = 1;
            const double expiryDelay = 1;

            const int key = 42;
            osg::ref_ptr<Object> value(new Object);
            cache->addEntryToObjectCache(key, value);
            value = nullptr;

            cache->update(referenceTime, expiryDelay);
            ASSERT_THAT(cache->getRefFromObjectCacheOrNone(key), Optional(_));
            ASSERT_EQ(cache->getStats().mExpired, 0);

            cache->update(referenceTime + expiryDelay, expiryDelay);
            EXPECT_EQ(cache->getRefFromObjectCacheOrNone(key), std::nullopt);
            ASSERT_EQ(cache->getStats().mExpired, 1);
        }

        TEST(ResourceGenericObjectCacheTest, updateShouldKeepExternallyReferencedItems)
        {
            osg::ref_ptr<GenericObjectCache<int>> cache(new GenericObjectCache<int>);

            const double referenceTime = 1;
            const double expiryDelay = 1;

            const int key = 42;
            osg::ref_ptr<Object> value(new Object);
            cache->addEntryToObjectCache(key, value);

            cache->update(referenceTime, expiryDelay);
            ASSERT_THAT(cache->getRefFromObjectCacheOrNone(key), Optional(_));

            cache->update(referenceTime + expiryDelay, expiryDelay);
            EXPECT_THAT(cache->getRefFromObjectCacheOrNone(key), Optional(value));
        }

        TEST(ResourceGenericObjectCacheTest, updateShouldKeepNotExpiredItems)
        {
            osg::ref_ptr<GenericObjectCache<int>> cache(new GenericObjectCache<int>);

            const double referenceTime = 1;
            const double expiryDelay = 2;

            const int key = 42;
            osg::ref_ptr<Object> value(new Object);
            cache->addEntryToObjectCache(key, value);
            value = nullptr;

            cache->update(referenceTime + expiryDelay, expiryDelay);
            ASSERT_THAT(cache->getRefFromObjectCacheOrNone(key), Optional(_));

            cache->update(referenceTime + expiryDelay / 2, expiryDelay);
            EXPECT_THAT(cache->getRefFromObjectCacheOrNone(key), Optional(_));
        }

        TEST(ResourceGenericObjectCacheTest, updateShouldKeepNotExpiredNullptrItems)
        {
            osg::ref_ptr<GenericObjectCache<int>> cache(new GenericObjectCache<int>);

            const double referenceTime = 1;
            const double expiryDelay = 2;

            const int key = 42;
            cache->addEntryToObjectCache(key, nullptr);

            cache->update(referenceTime + expiryDelay, expiryDelay);
            ASSERT_THAT(cache->getRefFromObjectCacheOrNone(key), Optional(_));

            cache->update(referenceTime + expiryDelay / 2, expiryDelay);
            EXPECT_THAT(cache->getRefFromObjectCacheOrNone(key), Optional(_));
        }

        TEST(ResourceGenericObjectCacheTest, getRefFromObjectCacheOrNoneShouldNotExtendItemLifetime)
        {
            osg::ref_ptr<GenericObjectCache<int>> cache(new GenericObjectCache<int>);

            const double referenceTime = 1;
            const double expiryDelay = 2;

            const int key = 42;
            cache->addEntryToObjectCache(key, nullptr);

            cache->update(referenceTime, expiryDelay);
            ASSERT_THAT(cache->getRefFromObjectCacheOrNone(key), Optional(_));

            cache->update(referenceTime + expiryDelay / 2, expiryDelay);
            ASSERT_THAT(cache->getRefFromObjectCacheOrNone(key), Optional(_));

            cache->update(referenceTime + expiryDelay, expiryDelay);
            EXPECT_EQ(cache->getRefFromObjectCacheOrNone(key), std::nullopt);
        }

        TEST(ResourceGenericObjectCacheTest, lowerBoundShouldSupportHeterogeneousLookup)
        {
            osg::ref_ptr<GenericObjectCache<std::string>> cache(new GenericObjectCache<std::string>);
            cache->addEntryToObjectCache("a", nullptr);
            cache->addEntryToObjectCache("c", nullptr);
            EXPECT_THAT(cache->lowerBound(std::string_view("b")), Optional(Pair("c", _)));
        }

        TEST(ResourceGenericObjectCacheTest, shouldSupportRemovingItems)
        {
            osg::ref_ptr<GenericObjectCache<int>> cache(new GenericObjectCache<int>);
            const int key = 42;
            osg::ref_ptr<Object> value(new Object);
            cache->addEntryToObjectCache(key, value);
            ASSERT_EQ(cache->getRefFromObjectCache(key), value);
            cache->removeFromObjectCache(key);
            EXPECT_EQ(cache->getRefFromObjectCacheOrNone(key), std::nullopt);
        }

        TEST(ResourceGenericObjectCacheTest, clearShouldRemoveAllItems)
        {
            osg::ref_ptr<GenericObjectCache<int>> cache(new GenericObjectCache<int>);

            const int key1 = 42;
            const int key2 = 13;
            osg::ref_ptr<Object> value1(new Object);
            osg::ref_ptr<Object> value2(new Object);
            cache->addEntryToObjectCache(key1, value1);
            cache->addEntryToObjectCache(key2, value2);

            ASSERT_EQ(cache->getRefFromObjectCache(key1), value1);
            ASSERT_EQ(cache->getRefFromObjectCache(key2), value2);

            cache->clear();

            EXPECT_EQ(cache->getRefFromObjectCacheOrNone(key1), std::nullopt);
            EXPECT_EQ(cache->getRefFromObjectCacheOrNone(key2), std::nullopt);
        }

        TEST(ResourceGenericObjectCacheTest, callShouldIterateOverAllItems)
        {
            osg::ref_ptr<GenericObjectCache<int>> cache(new GenericObjectCache<int>);

            osg::ref_ptr<Object> value1(new Object);
            osg::ref_ptr<Object> value2(new Object);
            osg::ref_ptr<Object> value3(new Object);
            cache->addEntryToObjectCache(1, value1);
            cache->addEntryToObjectCache(2, value2);
            cache->addEntryToObjectCache(3, value3);

            std::vector<std::pair<int, osg::Object*>> actual;
            cache->call([&](int key, osg::Object* value) { actual.emplace_back(key, value); });

            EXPECT_THAT(actual, ElementsAre(Pair(1, value1.get()), Pair(2, value2.get()), Pair(3, value3.get())));
        }

        TEST(ResourceGenericObjectCacheTest, getStatsShouldReturnNumberOrAddedItems)
        {
            osg::ref_ptr<GenericObjectCache<int>> cache(new GenericObjectCache<int>);

            osg::ref_ptr<Object> value1(new Object);
            osg::ref_ptr<Object> value2(new Object);
            cache->addEntryToObjectCache(13, value1);
            cache->addEntryToObjectCache(42, value2);

            const CacheStats stats = cache->getStats();

            EXPECT_EQ(stats.mSize, 2);
        }

        TEST(ResourceGenericObjectCacheTest, getStatsShouldReturnNumberOrGetsAndHits)
        {
            osg::ref_ptr<GenericObjectCache<int>> cache(new GenericObjectCache<int>);

            {
                const CacheStats stats = cache->getStats();

                EXPECT_EQ(stats.mGet, 0);
                EXPECT_EQ(stats.mHit, 0);
            }

            osg::ref_ptr<Object> value(new Object);
            cache->addEntryToObjectCache(13, value);
            cache->getRefFromObjectCache(13);
            cache->getRefFromObjectCache(42);

            {
                const CacheStats stats = cache->getStats();

                EXPECT_EQ(stats.mGet, 2);
                EXPECT_EQ(stats.mHit, 1);
            }
        }

        TEST(ResourceGenericObjectCacheTest, lowerBoundShouldReturnFirstNotLessThatGivenKey)
        {
            osg::ref_ptr<GenericObjectCache<int>> cache(new GenericObjectCache<int>);

            osg::ref_ptr<Object> value1(new Object);
            osg::ref_ptr<Object> value2(new Object);
            osg::ref_ptr<Object> value3(new Object);
            cache->addEntryToObjectCache(1, value1);
            cache->addEntryToObjectCache(2, value2);
            cache->addEntryToObjectCache(4, value3);

            EXPECT_THAT(cache->lowerBound(3), Optional(Pair(4, value3)));
        }

        TEST(ResourceGenericObjectCacheTest, lowerBoundShouldReturnNulloptWhenKeyIsGreaterThanAnyOther)
        {
            osg::ref_ptr<GenericObjectCache<int>> cache(new GenericObjectCache<int>);

            osg::ref_ptr<Object> value1(new Object);
            osg::ref_ptr<Object> value2(new Object);
            osg::ref_ptr<Object> value3(new Object);
            cache->addEntryToObjectCache(1, value1);
            cache->addEntryToObjectCache(2, value2);
            cache->addEntryToObjectCache(3, value3);

            EXPECT_EQ(cache->lowerBound(4), std::nullopt);
        }

        TEST(ResourceGenericObjectCacheTest, addEntryToObjectCacheShouldSupportHeterogeneousLookup)
        {
            osg::ref_ptr<GenericObjectCache<std::string>> cache(new GenericObjectCache<std::string>);
            const std::string key = "key";
            osg::ref_ptr<Object> value(new Object);
            cache->addEntryToObjectCache(std::string_view("key"), value);
            EXPECT_EQ(cache->getRefFromObjectCache(key), value);
        }

        TEST(ResourceGenericObjectCacheTest, addEntryToObjectCacheShouldKeyMoving)
        {
            osg::ref_ptr<GenericObjectCache<std::string>> cache(new GenericObjectCache<std::string>);
            std::string key(128, 'a');
            osg::ref_ptr<Object> value(new Object);
            cache->addEntryToObjectCache(std::move(key), value);
            EXPECT_EQ(key, "");
            EXPECT_EQ(cache->getRefFromObjectCache(std::string(128, 'a')), value);
        }

        TEST(ResourceGenericObjectCacheTest, removeFromObjectCacheShouldSupportHeterogeneousLookup)
        {
            osg::ref_ptr<GenericObjectCache<std::string>> cache(new GenericObjectCache<std::string>);
            const std::string key = "key";
            osg::ref_ptr<Object> value(new Object);
            cache->addEntryToObjectCache(key, value);
            ASSERT_EQ(cache->getRefFromObjectCache(key), value);
            cache->removeFromObjectCache(std::string_view("key"));
            EXPECT_EQ(cache->getRefFromObjectCacheOrNone(key), std::nullopt);
        }

        TEST(ResourceGenericObjectCacheTest, getRefFromObjectCacheShouldSupportHeterogeneousLookup)
        {
            osg::ref_ptr<GenericObjectCache<std::string>> cache(new GenericObjectCache<std::string>);
            const std::string key = "key";
            osg::ref_ptr<Object> value(new Object);
            cache->addEntryToObjectCache(key, value);
            EXPECT_EQ(cache->getRefFromObjectCache(std::string_view("key")), value);
        }

        TEST(ResourceGenericObjectCacheTest, getRefFromObjectCacheOrNoneShouldSupportHeterogeneousLookup)
        {
            osg::ref_ptr<GenericObjectCache<std::string>> cache(new GenericObjectCache<std::string>);
            const std::string key = "key";
            osg::ref_ptr<Object> value(new Object);
            cache->addEntryToObjectCache(key, value);
            EXPECT_THAT(cache->getRefFromObjectCacheOrNone(std::string_view("key")), Optional(value));
        }

        TEST(ResourceGenericObjectCacheTest, checkInObjectCacheShouldSupportHeterogeneousLookup)
        {
            osg::ref_ptr<GenericObjectCache<std::string>> cache(new GenericObjectCache<std::string>);
            const std::string key = "key";
            osg::ref_ptr<Object> value(new Object);
            cache->addEntryToObjectCache(key, value);
            EXPECT_TRUE(cache->checkInObjectCache(std::string_view("key"), 0));
        }
    }
}
