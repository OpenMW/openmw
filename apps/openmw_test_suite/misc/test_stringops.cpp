#include <gtest/gtest.h>
#include "components/misc/stringops.hpp"

struct PartialBinarySearchTest : public ::testing::Test
{
  protected:
    std::vector<std::string> mDataVec;
    virtual void SetUp()
    {
        const char* data[] = { "Head", "Chest", "Tri Head", "Tri Chest", "Bip01" };
        mDataVec = std::vector<std::string>(data, data+sizeof(data)/sizeof(data[0]));
        std::sort(mDataVec.begin(), mDataVec.end(), Misc::StringUtils::ciLess);
    }

    virtual void TearDown()
    {
    }

    bool matches(const std::string& keyword)
    {
        return Misc::StringUtils::partialBinarySearch(mDataVec.begin(), mDataVec.end(), keyword) != mDataVec.end();
    }
};

TEST_F(PartialBinarySearchTest, partial_binary_search_test)
{
    EXPECT_TRUE( matches("Head 01") );
    EXPECT_TRUE( matches("Head") );
    EXPECT_TRUE( matches("Tri Head 01") );
    EXPECT_TRUE( matches("Tri Head") );
    EXPECT_TRUE( matches("tri head") );

    EXPECT_FALSE( matches("Hea") );
    EXPECT_FALSE( matches(" Head") );
    EXPECT_FALSE( matches("Tri  Head") );
}

TEST_F (PartialBinarySearchTest, ci_test)
{
    EXPECT_TRUE (Misc::StringUtils::lowerCase("ASD") == "asd");

    // test to make sure system locale is not used
    std::string unicode1 = "\u04151 \u0418"; // CYRILLIC CAPITAL LETTER IE, CYRILLIC CAPITAL LETTER I
    EXPECT_TRUE( Misc::StringUtils::lowerCase(unicode1) == unicode1 );
}
