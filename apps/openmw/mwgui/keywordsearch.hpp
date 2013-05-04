#ifndef MWGUI_KEYWORDSEARCH_H
#define MWGUI_KEYWORDSEARCH_H

#include <map>
#include <locale>
#include <stdexcept>

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
        seed_impl  (/*std::move*/ (keyword), /*std::move*/ (value), 0, mRoot);
    }

    void clear ()
    {
        mRoot.mChildren.clear ();
        mRoot.mKeyword.clear ();
    }

    bool search (Point beg, Point end, Match & match)
    {
        for (Point i = beg; i != end; ++i)
        {
            // check first character
            typename Entry::childen_t::iterator candidate = mRoot.mChildren.find (std::tolower (*i, mLocale));

            // no match, on to next character
            if (candidate == mRoot.mChildren.end ())
                continue;

            // see how far the match goes
            Point j = i;

            while ((j + 1) != end)
            {
                typename Entry::childen_t::iterator next = candidate->second.mChildren.find (std::tolower (*++j, mLocale));

                if (next == candidate->second.mChildren.end ())
                    break;

                candidate = next;
            }

            // didn't match enough to disambiguate, on to next character
            if (!candidate->second.mKeyword.size ())
                continue;

            // match the rest of the keyword
            typename string_t::const_iterator t = candidate->second.mKeyword.begin () + (j - i);

            while (j != end && t != candidate->second.mKeyword.end ())
            {
                if (std::tolower (*j, mLocale) != std::tolower (*t, mLocale))
                    break;

                ++j, ++t;
            }

            // didn't match full keyword, on to next character
            if (t != candidate->second.mKeyword.end ())
                continue;

            // we did it, report the good news
            match.mValue = candidate->second.mValue;
            match.mBeg = i;
            match.mEnd = j;

            return true;
        }

        // no match in range, report the bad news
        return false;
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
        int ch = tolower (keyword.at (depth), mLocale);

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

                j->second.mKeyword.clear ();

                if (depth >= pushKeyword.size ())
                    throw std::runtime_error ("unexpected");

                if (depth+1 < pushKeyword.size())
                    seed_impl (/*std::move*/ (pushKeyword), /*std::move*/ (pushValue), depth+1, j->second);
            }

            if (depth+1 < keyword.size())
                seed_impl (/*std::move*/ (keyword), /*std::move*/ (value), depth+1, j->second);
        }

    }

    Entry mRoot;
    std::locale mLocale;
};

#endif
