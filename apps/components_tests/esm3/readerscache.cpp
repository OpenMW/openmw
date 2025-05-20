#include <components/esm3/readerscache.hpp>
#include <components/files/collections.hpp>
#include <components/files/multidircollection.hpp>

#include <gtest/gtest.h>

#ifndef OPENMW_DATA_DIR
#error "OPENMW_DATA_DIR is not defined"
#endif

namespace
{
    using namespace testing;
    using namespace ESM;

    TEST(ESM3ReadersCache, onAttemptToRequestTheSameReaderTwiceShouldThrowException)
    {
        ReadersCache readers(1);
        const ReadersCache::BusyItem reader = readers.get(0);
        EXPECT_THROW(readers.get(0), std::logic_error);
    }

    TEST(ESM3ReadersCache, shouldAllowToHaveBusyItemsMoreThanCapacity)
    {
        ReadersCache readers(1);
        const ReadersCache::BusyItem reader0 = readers.get(0);
        const ReadersCache::BusyItem reader1 = readers.get(1);
    }

    TEST(ESM3ReadersCache, shouldKeepClosedReleasedClosedItem)
    {
        ReadersCache readers(1);
        readers.get(0);
        const ReadersCache::BusyItem reader = readers.get(0);
        EXPECT_FALSE(reader->isOpen());
    }

    struct ESM3ReadersCacheWithContentFile : Test
    {
        static constexpr std::size_t sInitialOffset = 324;
        static constexpr std::size_t sSkip = 100;
        const Files::PathContainer mDataDirs{ { std::filesystem::path{ OPENMW_DATA_DIR } } };
        const Files::Collections mFileCollections{ mDataDirs };
        const std::string mContentFile = "template.omwgame";
        const std::filesystem::path mContentFilePath = mFileCollections.getCollection(".omwgame").getPath(mContentFile);
    };

    TEST_F(ESM3ReadersCacheWithContentFile, shouldKeepOpenReleasedOpenReader)
    {
        ReadersCache readers(1);
        {
            const ReadersCache::BusyItem reader = readers.get(0);
            reader->open(mContentFilePath);
            ASSERT_TRUE(reader->isOpen());
            ASSERT_EQ(reader->getFileOffset(), sInitialOffset);
            ASSERT_GT(reader->getFileSize(), sInitialOffset + sSkip);
            reader->skip(sSkip);
            ASSERT_EQ(reader->getFileOffset(), sInitialOffset + sSkip);
        }
        {
            const ReadersCache::BusyItem reader = readers.get(0);
            EXPECT_TRUE(reader->isOpen());
            EXPECT_EQ(reader->getName(), mContentFilePath);
            EXPECT_EQ(reader->getFileOffset(), sInitialOffset + sSkip);
        }
    }

    TEST_F(ESM3ReadersCacheWithContentFile, shouldCloseFreeReaderWhenReachingCapacityLimit)
    {
        ReadersCache readers(1);
        {
            const ReadersCache::BusyItem reader = readers.get(0);
            reader->open(mContentFilePath);
            ASSERT_TRUE(reader->isOpen());
            ASSERT_EQ(reader->getFileOffset(), sInitialOffset);
            ASSERT_GT(reader->getFileSize(), sInitialOffset + sSkip);
            reader->skip(sSkip);
            ASSERT_EQ(reader->getFileOffset(), sInitialOffset + sSkip);
        }
        {
            const ReadersCache::BusyItem reader = readers.get(1);
            reader->open(mContentFilePath);
            ASSERT_TRUE(reader->isOpen());
        }
        {
            const ReadersCache::BusyItem reader = readers.get(0);
            EXPECT_TRUE(reader->isOpen());
            EXPECT_EQ(reader->getFileOffset(), sInitialOffset);
        }
    }

    TEST_F(ESM3ReadersCacheWithContentFile, CachedSizeAndName)
    {
        ESM::ReadersCache readers(2);
        {
            readers.get(0)->openRaw(std::make_unique<std::istringstream>("123"), "closed0.omwaddon");
            readers.get(1)->openRaw(std::make_unique<std::istringstream>("12345"), "closed1.omwaddon");
            readers.get(2)->openRaw(std::make_unique<std::istringstream>("1234567"), "free.omwaddon");
        }
        auto busy = readers.get(3);
        busy->openRaw(std::make_unique<std::istringstream>("123456789"), "busy.omwaddon");

        EXPECT_EQ(readers.getFileSize(0), 3);
        EXPECT_EQ(readers.getName(0), "closed0.omwaddon");

        EXPECT_EQ(readers.getFileSize(1), 5);
        EXPECT_EQ(readers.getName(1), "closed1.omwaddon");

        EXPECT_EQ(readers.getFileSize(2), 7);
        EXPECT_EQ(readers.getName(2), "free.omwaddon");

        EXPECT_EQ(readers.getFileSize(3), 9);
        EXPECT_EQ(readers.getName(3), "busy.omwaddon");

        // not-yet-seen indices give zero for their size
        EXPECT_EQ(readers.getFileSize(4), 0);
    }
}
