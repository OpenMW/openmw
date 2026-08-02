#include "apps/openmw/mwdialogue/keywordsearch.hpp"

#include <components/translation/translation.hpp>

#include <gtest/gtest.h>

struct KeywordSearchTest : public ::testing::Test
{
protected:
    void SetUp() override {}

    void TearDown() override {}
};

TEST_F(KeywordSearchTest, keyword_test_conflict_resolution)
{
    // test to make sure the longest keyword in a chain of conflicting keywords gets chosen
    MWDialogue::KeywordSearch search;
    search.seed("foo bar", {});
    search.seed("bar lock", {});
    search.seed("lock switch", {});

    std::string text = "foo bar lock switch";

    std::vector<MWDialogue::KeywordSearch::Match> matches;
    search.highlightKeywords(text.begin(), text.end(), matches);

    // Should contain: "foo bar", "lock switch"
    EXPECT_EQ(matches.size(), 2);
    EXPECT_EQ(std::string(matches.front().mBeg, matches.front().mEnd), "foo bar");
    EXPECT_EQ(std::string(matches.rbegin()->mBeg, matches.rbegin()->mEnd), "lock switch");
}

TEST_F(KeywordSearchTest, keyword_test_conflict_resolution2)
{
    MWDialogue::KeywordSearch search;
    search.seed("the dwemer", {});
    search.seed("dwemer language", {});

    std::string text = "the dwemer language";

    std::vector<MWDialogue::KeywordSearch::Match> matches;
    search.highlightKeywords(text.begin(), text.end(), matches);

    EXPECT_EQ(matches.size(), 1);
    EXPECT_EQ(std::string(matches.front().mBeg, matches.front().mEnd), "dwemer language");
}

TEST_F(KeywordSearchTest, keyword_test_conflict_resolution3)
{
    // Test that the longest keyword is chosen, rather than maximizing the
    // amount of highlighted characters by highlighting the first and last keyword
    MWDialogue::KeywordSearch search;
    search.seed("foo bar", {});
    search.seed("bar lock", {});
    search.seed("lock so", {});

    std::string text = "foo bar lock so";

    std::vector<MWDialogue::KeywordSearch::Match> matches;
    search.highlightKeywords(text.begin(), text.end(), matches);

    EXPECT_EQ(matches.size(), 1);
    EXPECT_EQ(std::string(matches.front().mBeg, matches.front().mEnd), "bar lock");
}

TEST_F(KeywordSearchTest, keyword_test_utf8_word_begin)
{
    // Make sure that the search works well on UTF-8 strings containing some non-ASCII (French)
    MWDialogue::KeywordSearch search;
    search.seed("états", {});
    search.seed("ïrradiés", {});
    search.seed("ça nous déçois", {});
    search.seed("nous", {});

    std::string text
        = "les nations unis ont réunis le monde entier, états units inclus pour parler du problème des gens ïrradiés "
          "et ça nous déçois";

    std::vector<MWDialogue::KeywordSearch::Match> matches;
    search.highlightKeywords(text.begin(), text.end(), matches);

    EXPECT_EQ(matches.size(), 3);
    EXPECT_EQ(std::string(matches[0].mBeg, matches[0].mEnd), "états");
    EXPECT_EQ(std::string(matches[1].mBeg, matches[1].mEnd), "ïrradiés");
    EXPECT_EQ(std::string(matches[2].mBeg, matches[2].mEnd), "ça nous déçois");
}

TEST_F(KeywordSearchTest, keyword_test_non_alpha_non_whitespace_word_begin)
{
    // Make sure that the search works well even if the separator is not whitespace
    MWDialogue::KeywordSearch search;
    search.seed("Report to caius cosades", {});

    std::string text = "I was told to \"Report to Caius Cosades\"";

    std::vector<MWDialogue::KeywordSearch::Match> matches;
    search.highlightKeywords(text.begin(), text.end(), matches);

    EXPECT_EQ(matches.size(), 1);
    EXPECT_EQ(std::string(matches[0].mBeg, matches[0].mEnd), "Report to Caius Cosades");
}

TEST_F(KeywordSearchTest, keyword_test_russian_ascii_before)
{
    // Make sure that the search works well even if the separator is not whitespace with Russian chars
    MWDialogue::KeywordSearch search;
    search.seed("Доложить Каю Косадесу", {});

    std::string text
        = "Что? Да. Я Кай Косадес. То есть как это, вам велели 'Доложить Каю Косадесу'? О чем вы говорите?";

    std::vector<MWDialogue::KeywordSearch::Match> matches;
    search.highlightKeywords(text.begin(), text.end(), matches);

    EXPECT_EQ(matches.size(), 1);
    EXPECT_EQ(std::string(matches[0].mBeg, matches[0].mEnd), "Доложить Каю Косадесу");
}

TEST_F(KeywordSearchTest, keyword_test_substrings_without_word_separators)
{
    // Make sure that the search does not highlight substrings within words
    // i.e. "Force" does not contain "orc"
    // and "bring" does not contain "ring"
    MWDialogue::KeywordSearch search;
    search.seed("orc", {});
    search.seed("ring", {});

    std::string text = "Bring the Force, Lucan!";

    std::vector<MWDialogue::KeywordSearch::Match> matches;
    search.highlightKeywords(text.begin(), text.end(), matches);

    EXPECT_EQ(matches.size(), 0);
}

TEST_F(KeywordSearchTest, keyword_test_initial_substrings_match)
{
    // Make sure that the search highlights prefix substrings
    // "Orcs" should match "orc"
    // "ring" is not matched because "-" is not a word separator
    MWDialogue::KeywordSearch search;
    search.seed("orc", {});
    search.seed("ring", {});

    std::string text = "Bring the Orcs some gold-rings.";

    std::vector<MWDialogue::KeywordSearch::Match> matches;
    search.highlightKeywords(text.begin(), text.end(), matches);

    EXPECT_EQ(matches.size(), 1);
    EXPECT_EQ(std::string(matches[0].mBeg, matches[0].mEnd), "Orc");
}

TEST_F(KeywordSearchTest, keyword_test_polish_word_separators)
{
    // The Polish version supports ( and [ as word separators. The English version doesn't, but refuses to highlight
    // topics starting with those characters anyway
    MWDialogue::KeywordSearch search;
    search.seed("Jim Stacey", {});
    search.seed("gildia wojowników", {});

    std::string text
        = "Dżentelmen (Jim Stacey) kazał mi zabić człowieka imieniem Sjoring Kamienne Serce, mistrza gildii wojowników "
          "[gildia wojowników] w mieście Vivek.";

    std::vector<MWDialogue::KeywordSearch::Match> matches;
    search.highlightKeywords(text.begin(), text.end(), matches);

    ASSERT_EQ(matches.size(), 2);
    EXPECT_EQ(std::string(matches[0].mBeg, matches[0].mEnd), "Jim Stacey");
    EXPECT_EQ(std::string(matches[1].mBeg, matches[1].mEnd), "gildia wojowników");
}

TEST_F(KeywordSearchTest, keyword_test_french_substrings)
{
    // Substrings within words should not match
    MWDialogue::KeywordSearch search;
    search.seed("ages", {});
    search.seed("orc", {});

    std::string text = "traçages et forces";

    std::vector<MWDialogue::KeywordSearch::Match> matches;
    search.highlightKeywords(text.begin(), text.end(), matches);

    EXPECT_EQ(matches.size(), 0);
}

TEST_F(KeywordSearchTest, keyword_test_single_char_strings)
{
    // It should be possible to match a single character
    MWDialogue::KeywordSearch search;
    search.seed("AB", {});
    search.seed("a", {});

    std::string text = "a ab";

    std::vector<MWDialogue::KeywordSearch::Match> matches;
    search.highlightKeywords(text.begin(), text.end(), matches);

    EXPECT_EQ(matches.size(), 2);
    EXPECT_EQ(std::string(matches[0].mBeg, matches[0].mEnd), "a");
    EXPECT_EQ(std::string(matches[1].mBeg, matches[1].mEnd), "ab");
}

TEST_F(KeywordSearchTest, parse_hypertext_explicit_links)
{
    // Verify that @...# sequences are parsed as explicit links
    MWDialogue::KeywordSearch search;
    Translation::Storage storage;

    std::string text = "Go to @Balmora#.";
    auto matches = search.parseHyperText(text, storage);

    ASSERT_EQ(matches.size(), 1);
    EXPECT_TRUE(matches[0].mExplicit);
    EXPECT_EQ(matches[0].mTopicId, "balmora");
    EXPECT_EQ(matches[0].getDisplayName(), "Balmora");

    std::string matchedText(matches[0].mBeg, matches[0].mEnd);
    EXPECT_EQ(matchedText, "@Balmora#");
}

TEST_F(KeywordSearchTest, parse_hypertext_mixed_implicit_and_explicit)
{
    // Verify that explicit links and seeded keywords are both returned
    MWDialogue::KeywordSearch search;
    Translation::Storage storage;

    search.seed("guild", "mages guild");

    std::string text = "To @join# the guild, talk to the head.";
    auto matches = search.parseHyperText(text, storage);

    ASSERT_EQ(matches.size(), 2);

    EXPECT_TRUE(matches[0].mExplicit);
    EXPECT_EQ(matches[0].getDisplayName(), "join");
    EXPECT_EQ(matches[0].mTopicId, "join");
    EXPECT_FALSE(matches[1].mExplicit);
    EXPECT_EQ(matches[1].getDisplayName(), "guild");
    EXPECT_EQ(matches[1].mTopicId, "mages guild");
}

TEST_F(KeywordSearchTest, parse_hypertext_keywords_ignored_inside_explicit)
{
    // Verify that a seeded keyword is NOT detected if it sits inside an explicit tag
    MWDialogue::KeywordSearch search;
    Translation::Storage storage;

    search.seed("test", "test_id");

    std::string text = "This is a @test#.";
    auto matches = search.parseHyperText(text, storage);

    ASSERT_EQ(matches.size(), 1);
    EXPECT_TRUE(matches[0].mExplicit);
    EXPECT_EQ(matches[0].mTopicId, "test");
}

TEST_F(KeywordSearchTest, parse_hypertext_broken_tags)
{
    // Verify behavior when tags are malformed
    MWDialogue::KeywordSearch search;
    Translation::Storage storage;

    search.seed("text", "text_id");

    std::string text = "This is @broken text.";

    auto matches = search.parseHyperText(text, storage);

    ASSERT_EQ(matches.size(), 1);
    EXPECT_FALSE(matches[0].mExplicit);
    EXPECT_EQ(matches[0].mTopicId, "text_id");
}

TEST_F(KeywordSearchTest, parse_hypertext_explicit_with_translation)
{
    // Verify that standard form conversion works
    MWDialogue::KeywordSearch search;
    Translation::Storage storage;

    storage.addPhraseForm("Балмору", "Балмора");
    std::string text = "Поезжайте в @Балмору#.";

    auto matches = search.parseHyperText(text, storage);

    ASSERT_EQ(matches.size(), 1);
    EXPECT_TRUE(matches[0].mExplicit);
    EXPECT_EQ(matches[0].getDisplayName(), "Балмору");
    EXPECT_EQ(matches[0].mTopicId, "Балмора");
}

TEST_F(KeywordSearchTest, parse_hypertext_explicit_with_pseudo_asterisk_and_translation)
{
    // Verify that alternative forms are properly used
    MWDialogue::KeywordSearch search;
    Translation::Storage storage;

    storage.addPhraseForm("Guild", "Fighters Guild");
    storage.addPhraseForm("Guild*", "Mages Guild");

    std::string text = "Join the @Guild\x7F#.";

    auto matches = search.parseHyperText(text, storage);

    ASSERT_EQ(matches.size(), 1);
    EXPECT_EQ(matches[0].getDisplayName(), "Guild");
    EXPECT_EQ(matches[0].mTopicId, "mages guild");
}

TEST_F(KeywordSearchTest, parse_hypertext_explicit_pseudo_asterisks_only)
{
    // Verify the behavior of a link that *only* contains pseudo-asterisks
    MWDialogue::KeywordSearch search;
    Translation::Storage storage;
    storage.addPhraseForm("****", "tier two swear word");

    std::string text = "@\x7F\x7F\x7F\x7F#.";

    auto matches = search.parseHyperText(text, storage);

    ASSERT_EQ(matches.size(), 1);
    EXPECT_TRUE(matches[0].getDisplayName().empty());
    EXPECT_EQ(matches[0].mTopicId, "tier two swear word");
}

TEST_F(KeywordSearchTest, parse_hypertext_preserve_untranslated_links)
{
    // Verify that explicit hyperlinks without a standard form are preserved
    MWDialogue::KeywordSearch search;
    Translation::Storage storage;

    storage.addPhraseForm("Двемеры", "Двемер");

    std::string text = "@Двемеры# и @данмеры#.";

    auto matches = search.parseHyperText(text, storage);

    ASSERT_EQ(matches.size(), 2);
    EXPECT_EQ(matches[0].getDisplayName(), "Двемеры");
    EXPECT_EQ(matches[0].mTopicId, "Двемер");
    EXPECT_EQ(matches[1].getDisplayName(), "данмеры");
    EXPECT_EQ(matches[1].mTopicId, "данмеры");
}

TEST_F(KeywordSearchTest, parse_hypertext_mark_file_overrides_keyword)
{
    // The infamous Bloodmoon test case
    // The mark file overrides the keyword that matches the убит topic
    MWDialogue::KeywordSearch search;
    search.seed("assassinated", "убит");
    search.seed("proof", "доказательство");

    Translation::Storage storage;
    storage.addPhraseForm("доказательств", "доказательство");
    storage.addPhraseForm("убит", "убит");

    std::string text = "Конечно же, у меня нет @доказательств#, что он был убит or assassinated";

    auto matches = search.parseHyperText(text, storage);

    ASSERT_EQ(matches.size(), 2);
    EXPECT_TRUE(matches[0].mExplicit);
    EXPECT_EQ(matches[0].getDisplayName(), "доказательств");
    EXPECT_EQ(matches[0].mTopicId, "доказательство");
    EXPECT_FALSE(matches[1].mExplicit);
    EXPECT_EQ(matches[1].getDisplayName(), "assassinated");
    EXPECT_EQ(matches[1].mTopicId, "убит");
}
