#ifndef MISC_UTF8ITER_HPP
#define MISC_UTF8ITER_HPP

#include <boost/tuple/tuple.hpp>

class utf8_stream
{
public:

    typedef uint32_t unicode_char;
    typedef unsigned char const * point;

    static const unicode_char sBadChar = 0xFFFFFFFF;

    utf8_stream (point begin, point end) :
        cur (begin), nxt (begin), end (end)
    {
    }

    utf8_stream (std::pair <point, point> range) :
        cur (range.first), nxt (range.first), end (range.second)
    {
    }

    bool eof () const
    {
        return cur == end;
    }

    point current () const
    {
        return cur;
    }

    unicode_char peek ()
    {
        if (cur == nxt)
            next ();
        return val;
    }

    unicode_char consume ()
    {
        if (cur == nxt)
            next ();
        cur = nxt;
        return val;
    }

    static std::pair <unicode_char, point> decode (point cur, point end)
    {
        if ((*cur & 0x80) == 0)
        {
            unicode_char chr = *cur++;

            return std::make_pair (chr, cur);
        }

        int octets;
        unicode_char chr;

        boost::tie (octets, chr) = octet_count (*cur++);

        if (octets > 5)
            return std::make_pair (sBadChar, cur);

        auto eoc = cur + octets;

        if (eoc > end)
            return std::make_pair (sBadChar, cur);

        while (cur != eoc)
        {
            if ((*cur & 0xC0) != 0x80) // check continuation mark
                return std::make_pair (sBadChar, cur);;

            chr = (chr << 6) | unicode_char ((*cur++) & 0x3F);
        }

        return std::make_pair (chr, cur);
    }

private:

    static std::pair <int, unicode_char> octet_count (unsigned char octet)
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
        boost::tie (val, nxt) = decode (nxt, end);
    }

    point cur;
    point nxt;
    point end;
    unicode_char val;
};

#endif