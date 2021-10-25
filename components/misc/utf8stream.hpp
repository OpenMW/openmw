#ifndef MISC_UTF8ITER_HPP
#define MISC_UTF8ITER_HPP

#include <cstring>
#include <string>
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

    static UnicodeChar toLowerUtf8(UnicodeChar ch)
    {
        // Russian alphabet
        if (ch >= 0x0410 && ch < 0x0430)
            return ch + 0x20;

        // Cyrillic IO character
        if (ch == 0x0401)
            return ch + 0x50;

        // Latin alphabet
        if (ch >= 0x41 && ch < 0x60)
            return ch + 0x20;

        // German characters
        if (ch == 0xc4 || ch == 0xd6 || ch == 0xdc)
            return ch + 0x20;
        if (ch == 0x1e9e)
            return 0xdf;

        // TODO: probably we will need to support characters from other languages

        return ch;
    }

    static std::string lowerCaseUtf8(const std::string& str)
    {
        if (str.empty())
            return str;

        // Decode string as utf8 characters, convert to lower case and pack them to string
        std::string out;
        Utf8Stream stream (str.c_str());
        while (!stream.eof ())
        {
            UnicodeChar character = toLowerUtf8(stream.peek());

            if (character <= 0x7f)
                out.append(1, static_cast<char>(character));
            else if (character <= 0x7ff)
            {
                out.append(1, static_cast<char>(0xc0 | ((character >> 6) & 0x1f)));
                out.append(1, static_cast<char>(0x80 | (character & 0x3f)));
            }
            else if (character <= 0xffff)
            {
                out.append(1, static_cast<char>(0xe0 | ((character >> 12) & 0x0f)));
                out.append(1, static_cast<char>(0x80 | ((character >> 6) & 0x3f)));
                out.append(1, static_cast<char>(0x80 | (character & 0x3f)));
            }
            else
            {
                out.append(1, static_cast<char>(0xf0 | ((character >> 18) & 0x07)));
                out.append(1, static_cast<char>(0x80 | ((character >> 12) & 0x3f)));
                out.append(1, static_cast<char>(0x80 | ((character >> 6) & 0x3f)));
                out.append(1, static_cast<char>(0x80 | (character & 0x3f)));
            }

            stream.consume();
        }

        return out;
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
