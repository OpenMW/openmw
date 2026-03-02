#include "keywordsearch.hpp"

#include <algorithm>
#include <stdexcept>

#include <components/misc/strings/algorithm.hpp>
#include <components/misc/strings/lower.hpp>
#include <components/translation/translation.hpp>

namespace
{
    std::string_view removePseudoAsterisks(std::string_view phrase)
    {
        std::size_t lastNonAsteriskPos = phrase.find_last_not_of('\x7F');
        if (lastNonAsteriskPos == std::string_view::npos)
            return {};
        return phrase.substr(0, lastNonAsteriskPos + 1);
    }
}

namespace MWDialogue
{
    void KeywordSearch::seed(std::string_view keyword, std::string_view value)
    {
        if (keyword.empty())
            return;
        buildTrie(keyword, value, 0, mRoot);
    }

    void KeywordSearch::clear()
    {
        mRoot.mChildren.clear();
        mRoot.mKeyword.clear();
    }

    void KeywordSearch::highlightKeywords(Point beg, Point end, std::vector<Match>& out) const
    {
        std::vector<Match> matches;
        for (Point i = beg; i != end; ++i)
        {
            if (i != beg)
            {
                Point prev = i;
                --prev;
                constexpr std::string_view wordSeparators = "\n\r \t'\"([";
                if (wordSeparators.find(*prev) == std::string_view::npos)
                    continue;
            }

            const Entry* current = &mRoot;
            for (Point it = i; it != end; ++it)
            {
                auto found = current->mChildren.find(Misc::StringUtils::toLower(*it));
                if (found == current->mChildren.end())
                    break;
                current = &found->second;
                if (!current->mKeyword.empty())
                {
                    std::string_view remainingText(it + 1, end);
                    std::string_view remainingKeyword = std::string_view(current->mKeyword).substr(it + 1 - i);
                    if (Misc::StringUtils::ciStartsWith(remainingText, remainingKeyword))
                        matches.emplace_back(i, i + current->mKeyword.size(), current->mTopicId);
                }
            }
        }
        // resolve overlapping keywords
        while (!matches.empty())
        {
            std::size_t longestKeywordSize = 0;
            auto longestKeyword = matches.begin();
            for (auto it = matches.begin(); it != matches.end(); ++it)
            {
                std::size_t size = it->mEnd - it->mBeg;
                if (size > longestKeywordSize)
                {
                    longestKeywordSize = size;
                    longestKeyword = it;
                }

                auto next = it + 1;

                if (next == matches.end())
                    break;

                if (it->mEnd <= next->mBeg)
                {
                    break; // no overlap
                }
            }

            Match keyword = *longestKeyword;
            matches.erase(longestKeyword);
            out.push_back(keyword);
            // erase anything that overlaps with the keyword we just added to the output
            std::erase_if(
                matches, [&](const Match& match) { return match.mBeg < keyword.mEnd && match.mEnd > keyword.mBeg; });
        }

        std::sort(out.begin(), out.end(), [](const Match& left, const Match& right) { return left.mBeg < right.mBeg; });
    }

    void KeywordSearch::buildTrie(std::string_view keyword, std::string_view topicId, size_t depth, Entry& entry)
    {
        const char ch = Misc::StringUtils::toLower(keyword[depth]);
        const auto found = entry.mChildren.find(ch);

        if (found == entry.mChildren.end())
        {
            entry.mChildren[ch].mTopicId = topicId;
            entry.mChildren[ch].mKeyword = keyword;
        }
        else
        {
            if (!found->second.mKeyword.empty())
            {
                std::string_view existingKeyword = found->second.mKeyword;
                if (Misc::StringUtils::ciEqual(keyword, existingKeyword))
                    return;
                if (depth >= existingKeyword.size())
                    throw std::runtime_error("unexpected trie depth");
                // Turn this Entry into a branch and append a leaf to hold its current ID
                if (depth + 1 < existingKeyword.size())
                {
                    buildTrie(existingKeyword, found->second.mTopicId, depth + 1, found->second);
                    found->second.mKeyword.clear();
                }
            }
            if (depth + 1 == keyword.size())
            {
                found->second.mTopicId = topicId;
                found->second.mKeyword = keyword;
            }
            else
            {
                buildTrie(keyword, topicId, depth + 1, found->second);
            }
        }
    }

    std::vector<KeywordSearch::Match> KeywordSearch::parseHyperText(
        const std::string& text, const Translation::Storage& storage) const
    {
        std::vector<Match> matches;
        size_t posEnd = std::string::npos;
        size_t iterationPos = 0;

        for (;;)
        {
            const size_t posBegin = text.find('@', iterationPos);
            if (posBegin != std::string::npos)
                posEnd = text.find('#', posBegin);

            if (posBegin != std::string::npos && posEnd != std::string::npos)
            {
                if (posBegin != iterationPos)
                    highlightKeywords(text.begin() + iterationPos, text.begin() + posBegin, matches);

                Match token;
                token.mExplicit = true;

                // This covers the entire link including the tags
                token.mBeg = text.begin() + posBegin;
                token.mEnd = text.begin() + posEnd + 1;

                const std::string_view id(token.mBeg + 1, token.mEnd - 1);
                token.mTopicId = removePseudoAsterisks(id);
                token.mTopicId.append(id.size() - token.mTopicId.size(), '*');
                token.mTopicId = Misc::StringUtils::lowerCase(storage.topicStandardForm(token.mTopicId));

                matches.push_back(std::move(token));

                iterationPos = posEnd + 1;
            }
            else
            {
                if (iterationPos < text.size())
                    highlightKeywords(text.begin() + iterationPos, text.end(), matches);
                break;
            }
        }

        return matches;
    }

    std::string_view KeywordSearch::Match::getDisplayName() const
    {
        if (mExplicit)
            return removePseudoAsterisks(std::string_view(mBeg + 1, mEnd - 1));
        return std::string_view(mBeg, mEnd);
    }
}
