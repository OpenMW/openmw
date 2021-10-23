#include <components/esmloader/load.cpp>
#include <components/files/collections.hpp>
#include <components/files/multidircollection.hpp>
#include <components/to_utf8/to_utf8.hpp>

#include <gtest/gtest.h>

#ifndef OPENMW_DATA_DIR
#error "OPENMW_DATA_DIR is not defined"
#endif

namespace
{
    using namespace testing;
    using namespace EsmLoader;

    struct EsmLoaderTest : Test
    {
        const Files::PathContainer mDataDirs {{std::string(OPENMW_DATA_DIR)}};
        const Files::Collections mFileCollections {mDataDirs, true};
        const std::vector<std::string> mContentFiles {{"template.omwgame"}};
    };

    TEST_F(EsmLoaderTest, loadEsmDataShouldSupportOmwgame)
    {
        Query query;
        query.mLoadActivators = true;
        query.mLoadCells = true;
        query.mLoadContainers = true;
        query.mLoadDoors = true;
        query.mLoadGameSettings = true;
        query.mLoadLands = true;
        query.mLoadStatics = true;
        std::vector<ESM::ESMReader> readers(mContentFiles.size());
        ToUTF8::Utf8Encoder* const encoder = nullptr;
        const EsmData esmData = loadEsmData(query, mContentFiles, mFileCollections, readers, encoder);
        EXPECT_EQ(esmData.mActivators.size(), 0);
        EXPECT_EQ(esmData.mCells.size(), 1);
        EXPECT_EQ(esmData.mContainers.size(), 0);
        EXPECT_EQ(esmData.mDoors.size(), 0);
        EXPECT_EQ(esmData.mGameSettings.size(), 1521);
        EXPECT_EQ(esmData.mLands.size(), 1);
        EXPECT_EQ(esmData.mStatics.size(), 2);
    }

    TEST_F(EsmLoaderTest, shouldIgnoreCellsWhenQueryLoadCellsIsFalse)
    {
        Query query;
        query.mLoadActivators = true;
        query.mLoadCells = false;
        query.mLoadContainers = true;
        query.mLoadDoors = true;
        query.mLoadGameSettings = true;
        query.mLoadLands = true;
        query.mLoadStatics = true;
        std::vector<ESM::ESMReader> readers(mContentFiles.size());
        ToUTF8::Utf8Encoder* const encoder = nullptr;
        const EsmData esmData = loadEsmData(query, mContentFiles, mFileCollections, readers, encoder);
        EXPECT_EQ(esmData.mActivators.size(), 0);
        EXPECT_EQ(esmData.mCells.size(), 0);
        EXPECT_EQ(esmData.mContainers.size(), 0);
        EXPECT_EQ(esmData.mDoors.size(), 0);
        EXPECT_EQ(esmData.mGameSettings.size(), 1521);
        EXPECT_EQ(esmData.mLands.size(), 1);
        EXPECT_EQ(esmData.mStatics.size(), 2);
    }

    TEST_F(EsmLoaderTest, shouldIgnoreCellsGameSettingsWhenQueryLoadGameSettingsIsFalse)
    {
        Query query;
        query.mLoadActivators = true;
        query.mLoadCells = true;
        query.mLoadContainers = true;
        query.mLoadDoors = true;
        query.mLoadGameSettings = false;
        query.mLoadLands = true;
        query.mLoadStatics = true;
        std::vector<ESM::ESMReader> readers(mContentFiles.size());
        ToUTF8::Utf8Encoder* const encoder = nullptr;
        const EsmData esmData = loadEsmData(query, mContentFiles, mFileCollections, readers, encoder);
        EXPECT_EQ(esmData.mActivators.size(), 0);
        EXPECT_EQ(esmData.mCells.size(), 1);
        EXPECT_EQ(esmData.mContainers.size(), 0);
        EXPECT_EQ(esmData.mDoors.size(), 0);
        EXPECT_EQ(esmData.mGameSettings.size(), 0);
        EXPECT_EQ(esmData.mLands.size(), 1);
        EXPECT_EQ(esmData.mStatics.size(), 2);
    }

    TEST_F(EsmLoaderTest, shouldIgnoreAllWithDefaultQuery)
    {
        const Query query;
        std::vector<ESM::ESMReader> readers(mContentFiles.size());
        ToUTF8::Utf8Encoder* const encoder = nullptr;
        const EsmData esmData = loadEsmData(query, mContentFiles, mFileCollections, readers, encoder);
        EXPECT_EQ(esmData.mActivators.size(), 0);
        EXPECT_EQ(esmData.mCells.size(), 0);
        EXPECT_EQ(esmData.mContainers.size(), 0);
        EXPECT_EQ(esmData.mDoors.size(), 0);
        EXPECT_EQ(esmData.mGameSettings.size(), 0);
        EXPECT_EQ(esmData.mLands.size(), 0);
        EXPECT_EQ(esmData.mStatics.size(), 0);
    }
}
