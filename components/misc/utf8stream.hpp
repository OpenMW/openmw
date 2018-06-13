#ifndef MISC_UTF8ITER_HPP
#define MISC_UTF8ITER_HPP

#include <cstring>
#include <tuple>

class Utf8Stream
{
public:

    typedef uint32_t UnicodeChar;
    typedef unsigned char const * Point;

    //static const unicode_char sBadChar = 0xFFFFFFFF; gcc can't handle this
    static UnicodeChar sBadChar () { return UnicodeChar (0xFFFFFFFF); }

    Utf8Stream (Point begin, Point end) :
        cur (begin), nxt (begin), end (end), val(Utf8Stream::sBadChar())
    {
    }

    Utf8Stream (const char * str) :
        cur ((unsigned char*) str), nxt ((unsigned char*) str), end ((unsigned char*) str + strlen(str)), val(Utf8Stream::sBadChar())
    {
    }

    Utf8Stream (std::pair <Point, Point> range) :
        cur (range.first), nxt (range.first), end (range.second), val(Utf8Stream::sBadChar())
    {
    }

    bool eof () const
    {
        return cur == end;
    }

    Point current () const
    {
        return cur;
    }

    UnicodeChar peek ()
    {
        if (cur == nxt)
            next ();
        return val;
    }

    UnicodeChar consume ()
    {
        if (cur == nxt)
            next ();
        cur = nxt;
        return val;
    }

    static std::pair <UnicodeChar, Point> decode (Point cur, Point end)
    {
        if ((*cur & 0x80) == 0)
        {
            UnicodeChar chr = *cur++;

            return std::make_pair (chr, cur);
        }

        int octets;
        UnicodeChar chr;

        std::tie (octets, chr) = octet_count (*cur++);

        if (octets > 5)
            return std::make_pair (sBadChar(), cur);

        Point eoc = cur + octets;

        if (eoc > end)
            return std::make_pair (sBadChar(), cur);

        while (cur != eoc)
        {
            if ((*cur & 0xC0) != 0x80) // check continuation mark
                return std::make_pair (sBadChar(), cur);

            chr = (chr << 6) | UnicodeChar ((*cur++) & 0x3F);
        }

        return std::make_pair (chr, cur);
    }

private:

    static std::pair <int, UnicodeChar> octet_count (unsigned char octet)
    {
        int octets;

        unsigned char mark = 0xC0;
        unsigned char mask = 0xE0;

        for (octets = 1; octets <= 5; ++octets)
        {
            if ((octet & mask) == mark)
                break;

            mark = (mark >> 1) | 0x80;
            mask = (mask >> 1) | 0x80;
        }

        return std::make_pair (octets, octet & ~mask);
    }

    void next ()
    {
        std::tie (val, nxt) = decode (nxt, end);
    }

    Point cur;
    Point nxt;
    Point end;
    UnicodeChar val;
};

#endif
