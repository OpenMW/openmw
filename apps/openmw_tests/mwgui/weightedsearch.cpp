#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <string>
#include <vector>

#include "apps/openmw/mwgui/weightedsearch.hpp"

namespace MWGui
{
    namespace
    {
        TEST(MWGuiWeightedSearchTests, weightedSearchShouldNotCrashWithLargeCorpus)
        {
            EXPECT_NO_THROW(weightedSearch(std::string(100000, 'x'), std::vector<std::string>{ "x" }));
        }
        TEST(MWGuiWeightedSearchTests, weightedSearchShouldReturn1WithEmptyPatternArray)
        {
            EXPECT_EQ(weightedSearch(std::string(100, 'x'), std::vector<std::string>{}), 1);
        }
        TEST(MWGuiWeightedSearchTests, weightedSearchShouldReturnTheSumOfAllPatternsWithAtLeastOneMatch)
        {
            EXPECT_EQ(weightedSearch(std::string("xyyzzz"), std::vector<std::string>{ "x", "y", "z" }), 3);
        }
        TEST(MWGuiWeightedSearchTests, generatePatternArrayShouldReturnEmptyArrayIfInputIsEmptyOrOnlySpaces)
        {
            EXPECT_THAT(generatePatternArray(std::string("")), testing::IsEmpty());
            EXPECT_THAT(generatePatternArray(std::string(10, ' ')), testing::IsEmpty());
        }
    }
}
