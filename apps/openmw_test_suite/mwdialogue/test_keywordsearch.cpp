#include <gtest/gtest.h>
#include "apps/openmw/mwdialogue/keywordsearch.hpp"

struct KeywordSearchTest : public ::testing::Test
{
  protected:
    void SetUp() override
    {
    }

    void TearDown() override
    {
    }
};

TEST_F(KeywordSearchTest, keyword_test_conflict_resolution)
{
    // test to make sure the longest keyword in a chain of conflicting keywords gets chosen
    MWDialogue::KeywordSearch<std::string, int> search;
    search.seed("foo bar", 0);
    search.seed("bar lock", 0);
    search.seed("lock switch", 0);

    std::string text = "foo bar lock switch";

    std::vector<MWDialogue::KeywordSearch<std::string, int>::Match> matches;
    search.highlightKeywords(text.begin(), text.end(), matches);

    // Should contain: "foo bar", "lock switch"
    EXPECT_EQ (matches.size() , 2);
    EXPECT_EQ (std::string(matches.front().mBeg, matches.front().mEnd) , "foo bar");
    EXPECT_EQ (std::string(matches.rbegin()->mBeg, matches.rbegin()->mEnd) , "lock switch");
}

TEST_F(KeywordSearchTest, keyword_test_conflict_resolution2)
{
    MWDialogue::KeywordSearch<std::string, int> search;
    search.seed("the dwemer", 0);
    search.seed("dwemer language", 0);

    std::string text = "the dwemer language";

    std::vector<MWDialogue::KeywordSearch<std::string, int>::Match> matches;
    search.highlightKeywords(text.begin(), text.end(), matches);

    EXPECT_EQ (matches.size() , 1);
    EXPECT_EQ (std::string(matches.front().mBeg, matches.front().mEnd) , "dwemer language");
}


TEST_F(KeywordSearchTest, keyword_test_conflict_resolution3)
{
    // testing that the longest keyword is chosen, rather than maximizing the
    // amount of highlighted characters by highlighting the first and last keyword
    MWDialogue::KeywordSearch<std::string, int> search;
    search.seed("foo bar", 0);
    search.seed("bar lock", 0);
    search.seed("lock so", 0);

    std::string text = "foo bar lock so";

    std::vector<MWDialogue::KeywordSearch<std::string, int>::Match> matches;
    search.highlightKeywords(text.begin(), text.end(), matches);

    EXPECT_EQ (matches.size() , 1);
    EXPECT_EQ (std::string(matches.front().mBeg, matches.front().mEnd) , "bar lock");
}


TEST_F(KeywordSearchTest, keyword_test_utf8_word_begin)
{
    // We make sure that the search works well even if the character is not ASCII
    MWDialogue::KeywordSearch<std::string, int> search;
    search.seed("états", 0);
    search.seed("ïrradiés", 0);
    search.seed("ça nous déçois", 0);


    std::string text = "les nations unis ont réunis le monde entier, états units inclus pour parler du problème des gens ïrradiés et ça nous déçois";

    std::vector<MWDialogue::KeywordSearch<std::string, int>::Match> matches;
    search.highlightKeywords(text.begin(), text.end(), matches);

    EXPECT_EQ (matches.size() , 3);
    EXPECT_EQ (std::string( matches[0].mBeg, matches[0].mEnd) , "états");
    EXPECT_EQ (std::string( matches[1].mBeg, matches[1].mEnd) , "ïrradiés");
    EXPECT_EQ (std::string( matches[2].mBeg, matches[2].mEnd) , "ça nous déçois");
}
