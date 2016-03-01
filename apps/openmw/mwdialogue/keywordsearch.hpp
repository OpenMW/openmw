#ifndef GAME_MWDIALOGUE_KEYWORDSEARCH_H
#define GAME_MWDIALOGUE_KEYWORDSEARCH_H

#include <map>
#include <cctype>
#include <stdexcept>
#include <vector>
#include <algorithm>    // std::reverse

#include <components/misc/stringops.hpp>

namespace MWDialogue
{

template <typename string_t, typename value_t>
class KeywordSearch
{
public:

    typedef typename string_t::const_iterator Point;

    struct Match
    {
        Point mBeg;
        Point mEnd;
        value_t mValue;
    };

    void seed (string_t keyword, value_t value)
    {
        if (keyword.empty())
            return;
        seed_impl  (/*std::move*/ (keyword), /*std::move*/ (value), 0, mRoot);
    }

    void clear ()
    {
        mRoot.mChildren.clear ();
        mRoot.mKeyword.clear ();
    }

    bool containsKeyword (string_t keyword, value_t& value)
    {
        typename Entry::childen_t::iterator current;
        typename Entry::childen_t::iterator next;

        current = mRoot.mChildren.find (Misc::StringUtils::toLower (*keyword.begin()));
        if (current == mRoot.mChildren.end())
            return false;
        else if (current->second.mKeyword.size() && Misc::StringUtils::ciEqual(current->second.mKeyword, keyword))
        {
            value = current->second.mValue;
            return true;
        }

        for (Point i = ++keyword.begin(); i != keyword.end(); ++i)
        {
            next = current->second.mChildren.find(Misc::StringUtils::toLower (*i));
            if (next == current->second.mChildren.end())
                return false;
            if (Misc::StringUtils::ciEqual(next->second.mKeyword, keyword))
            {
                value = next->second.mValue;
                return true;
            }
            current = next;
        }
        return false;
    }

    static bool sortMatches(const Match& left, const Match& right)
    {
        return left.mBeg < right.mBeg;
    }

    void highlightKeywords (Point beg, Point end, std::vector<Match>& out)
    {
        std::vector<Match> matches;
        for (Point i = beg; i != end; ++i)
        {
            // check if previous character marked start of new word
            if (i != beg)
            {
                Point prev = i;
                --prev;
                if(isalpha(*prev))
                    continue;
            }


            // check first character
            typename Entry::childen_t::iterator candidate = mRoot.mChildren.find (Misc::StringUtils::toLower (*i));

            // no match, on to next character
            if (candidate == mRoot.mChildren.end ())
                continue;

            // see how far the match goes
            Point j = i;

            // some keywords might be longer variations of other keywords, so we definitely need a list of candidates
            // the first element in the pair is length of the match, i.e. depth from the first character on
            std::vector< typename std::pair<int, typename Entry::childen_t::iterator> > candidates;

            while ((j + 1) != end)
            {
                typename Entry::childen_t::iterator next = candidate->second.mChildren.find (Misc::StringUtils::toLower (*++j));

                if (next == candidate->second.mChildren.end ())
                {
                    if (candidate->second.mKeyword.size() > 0)
                        candidates.push_back(std::make_pair((j-i), candidate));
                    break;
                }

                candidate = next;

                if (candidate->second.mKeyword.size() > 0)
                    candidates.push_back(std::make_pair((j-i), candidate));
            }

            if (candidates.empty())
                continue; // didn't match enough to disambiguate, on to next character

            // shorter candidates will be added to the vector first. however, we want to check against longer candidates first
            std::reverse(candidates.begin(), candidates.end());

            for (typename std::vector< std::pair<int, typename Entry::childen_t::iterator> >::iterator it = candidates.begin();
                 it != candidates.end(); ++it)
            {
                candidate = it->second;
                // try to match the rest of the keyword
                Point k = i + it->first;
                typename string_t::const_iterator t = candidate->second.mKeyword.begin () + (k - i);


                while (k != end && t != candidate->second.mKeyword.end ())
                {
                    if (Misc::StringUtils::toLower (*k) != Misc::StringUtils::toLower (*t))
                        break;

                    ++k, ++t;
                }

                // didn't match full keyword, try the next candidate
                if (t != candidate->second.mKeyword.end ())
                    continue;

                // found a keyword, but there might still be longer keywords that start somewhere _within_ this keyword
                // we will resolve these overlapping keywords later, choosing the longest one in case of conflict
                Match match;
                match.mValue = candidate->second.mValue;
                match.mBeg = i;
                match.mEnd = k;
                matches.push_back(match);
                break;
            }
        }

        // resolve overlapping keywords
        while (!matches.empty())
        {
            int longestKeywordSize = 0;
            typename std::vector<Match>::iterator longestKeyword = matches.begin();
            for (typename std::vector<Match>::iterator it = matches.begin(); it != matches.end(); ++it)
            {
                int size = it->mEnd - it->mBeg;
                if (size > longestKeywordSize)
                {
                    longestKeywordSize = size;
                    longestKeyword = it;
                }

                typename std::vector<Match>::iterator next = it;
                ++next;

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
            for (typename std::vector<Match>::iterator it = matches.begin(); it != matches.end();)
            {
                if (it->mBeg < keyword.mEnd && it->mEnd > keyword.mBeg)
                    it = matches.erase(it);
                else
                    ++it;
            }
        }

        std::sort(out.begin(), out.end(), sortMatches);
    }

private:

    struct Entry
    {
        typedef std::map <wchar_t, Entry> childen_t;

        string_t mKeyword;
        value_t mValue;
        childen_t mChildren;
    };

    void seed_impl (string_t keyword, value_t value, size_t depth, Entry  & entry)
    {
        int ch = Misc::StringUtils::toLower (keyword.at (depth));

        typename Entry::childen_t::iterator j = entry.mChildren.find (ch);

        if (j == entry.mChildren.end ())
        {
            entry.mChildren [ch].mValue = /*std::move*/ (value);
            entry.mChildren [ch].mKeyword = /*std::move*/ (keyword);
        }
        else
        {
            if (j->second.mKeyword.size () > 0)
            {
                if (keyword == j->second.mKeyword)
                    throw std::runtime_error ("duplicate keyword inserted");

                value_t pushValue = /*std::move*/ (j->second.mValue);
                string_t pushKeyword = /*std::move*/ (j->second.mKeyword);

                if (depth >= pushKeyword.size ())
                    throw std::runtime_error ("unexpected");

                if (depth+1 < pushKeyword.size())
                {
                    seed_impl (/*std::move*/ (pushKeyword), /*std::move*/ (pushValue), depth+1, j->second);
                    j->second.mKeyword.clear ();
                }
            }
            if (depth+1 == keyword.size())
                j->second.mKeyword = value;
            else // depth+1 < keyword.size()
                seed_impl (/*std::move*/ (keyword), /*std::move*/ (value), depth+1, j->second);
        }

    }

    Entry mRoot;
};

}

#endif
