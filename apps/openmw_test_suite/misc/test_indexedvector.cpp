#include <components/misc/indexedvector.hpp>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <cstdint>
#include <vector>

namespace
{
    TEST(IndexedVectorTests, basicInsert)
    {
        std::vector<std::pair<int, int>> vec;
        Misc::IndexedVector<int, int> map;

        for (int i = 0; i < 10000; i++)
        {
            vec.emplace_back(i, i << 13);
            map.emplace(i, i << 13);
        }

        ASSERT_EQ(vec.size(), map.size());

        auto itVec = vec.begin();
        auto itMap = map.begin();
        while (itMap != map.end() && itVec != vec.end())
        {
            ASSERT_EQ(itVec->first, itMap->first);
            ASSERT_EQ(itVec->second, itMap->second);
            itMap++;
            itVec++;
        }
    }

    TEST(IndexedVectorTests, duplicateInsert)
    {
        Misc::IndexedVector<int, int> map;

        auto pairVal = map.emplace(1, 5);
        ASSERT_EQ(map.size(), 1);
        ASSERT_EQ(pairVal.first, map.begin());
        ASSERT_EQ(pairVal.first->second, 5);
        ASSERT_EQ(pairVal.second, true);

        pairVal = map.emplace(1, 10);
        ASSERT_EQ(map.size(), 1);
        ASSERT_EQ(pairVal.first, map.begin());
        ASSERT_EQ(pairVal.first->second, 5);
        ASSERT_EQ(pairVal.second, false);
    }

    TEST(IndexedVectorTests, testErase)
    {
        Misc::IndexedVector<int, int> map;
        for (int i = 0; i < 10000; i++)
        {
            map.emplace(i, i);
        }

        auto itA = map.find(100);
        ASSERT_NE(itA, map.end());
        ASSERT_EQ(itA->second, 100);

        itA = map.erase(itA);
        ASSERT_EQ(map.size(), 10000 - 1);
        ASSERT_NE(itA, map.end());
        ASSERT_EQ(itA->second, 101);
    }

    TEST(IndexedVectorTests, testLookup)
    {
        Misc::IndexedVector<int, int> map;
        for (int i = 0; i < 10000; i++)
        {
            map.emplace(i, i);
        }

        auto itA = map.find(100);
        ASSERT_NE(itA, map.end());
        ASSERT_EQ(itA->second, 100);

        map.erase(itA);
        ASSERT_EQ(map.size(), 10000 - 1);
        itA = map.find(101);
        ASSERT_NE(itA, map.end());
        ASSERT_EQ(itA->second, 101);
    }
}
