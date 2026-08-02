#ifndef GAME_MWDIALOGUE_KEYWORDSEARCH_H
#define GAME_MWDIALOGUE_KEYWORDSEARCH_H

#include <map>
#include <string>
#include <string_view>
#include <vector>

namespace Translation
{
    class Storage;
}

namespace MWDialogue
{

    class KeywordSearch
    {
        struct Entry
        {
            std::string mKeyword;
            std::string mTopicId;
            std::map<char, Entry> mChildren;
        };

        void buildTrie(std::string_view keyword, std::string_view value, std::size_t depth, Entry& entry);
        Entry mRoot;

    public:
        using Point = std::string::const_iterator;

        struct Match
        {
            Point mBeg;
            Point mEnd;
            std::string mTopicId;
            bool mExplicit{ false };

            std::string_view getDisplayName() const;
        };

        void seed(std::string_view keyword, std::string_view value);
        void clear();
        void highlightKeywords(Point beg, Point end, std::vector<Match>& out) const;
        std::vector<KeywordSearch::Match> parseHyperText(
            const std::string& text, const Translation::Storage& storage) const;
    };

}

#endif
