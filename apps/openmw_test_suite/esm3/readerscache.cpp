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
}
