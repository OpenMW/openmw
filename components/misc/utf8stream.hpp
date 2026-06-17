#ifndef MISC_UTF8ITER_HPP
#define MISC_UTF8ITER_HPP

#include <cstdint>
#include <cstring>
#include <string>
#include <string_view>
#include <tuple>

class Utf8Stream
{
public:
    typedef uint32_t UnicodeChar;
    typedef unsigned char const* Point;

    // static const unicode_char sBadChar = 0xFFFFFFFF; gcc can't handle this
    static UnicodeChar sBadChar() { return UnicodeChar(0xFFFFFFFF); }

    Utf8Stream(Point begin, Point end)
        : mCur(begin)
        , mNxt(begin)
        , mEnd(end)
        , mVal(Utf8Stream::sBadChar())
    {
    }

    Utf8Stream(const char* str)
        : mCur(reinterpret_cast<const unsigned char*>(str))
        , mNxt(reinterpret_cast<const unsigned char*>(str))
        , mEnd(reinterpret_cast<const unsigned char*>(str) + strlen(str))
        , mVal(Utf8Stream::sBadChar())
    {
    }

    Utf8Stream(std::pair<Point, Point> range)
        : mCur(range.first)
        , mNxt(range.first)
        , mEnd(range.second)
        , mVal(Utf8Stream::sBadChar())
    {
    }

    Utf8Stream(std::string_view str)
        : Utf8Stream(reinterpret_cast<Point>(str.data()), reinterpret_cast<Point>(str.data() + str.size()))
    {
    }

    bool eof() const { return mCur == mEnd; }

    Point current() const { return mCur; }

    UnicodeChar peek()
    {
        if (mCur == mNxt)
            next();
        return mVal;
    }

    UnicodeChar consume()
    {
        if (mCur == mNxt)
            next();
        mCur = mNxt;
        return mVal;
    }

    static bool isAscii(unsigned char value) { return (value & 0x80) == 0; }

    static std::pair<UnicodeChar, Point> decode(Point cur, Point end)
    {
        if (isAscii(*cur))
        {
            UnicodeChar chr = *cur++;

            return std::make_pair(chr, cur);
        }

        std::size_t octets;
        UnicodeChar chr;

        std::tie(octets, chr) = getOctetCount(*cur++);

        return decode(cur, end, chr, octets);
    }

    static std::pair<UnicodeChar, Point> decode(Point cur, Point end, UnicodeChar chr, std::size_t octets)
    {
        if (octets > 5)
            return std::make_pair(sBadChar(), cur);

        Point eoc = cur + octets;

        if (eoc > end)
            return std::make_pair(sBadChar(), cur);

        while (cur != eoc)
        {
            if ((*cur & 0xC0) != 0x80) // check continuation mark
                return std::make_pair(sBadChar(), cur);

            chr = (chr << 6) | UnicodeChar((*cur++) & 0x3F);
        }

        return std::make_pair(chr, cur);
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

    static std::string lowerCaseUtf8(std::string_view str)
    {
        if (str.empty())
            return std::string{ str };

        // Decode string as utf8 characters, convert to lower case and pack them to string
        std::string out;
        out.reserve(str.length());
        Utf8Stream stream(str);
        while (!stream.eof())
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

    static std::pair<std::size_t, UnicodeChar> getOctetCount(unsigned char octet)
    {
        std::size_t octets;

        unsigned char mark = 0xC0;
        unsigned char mask = 0xE0;

        for (octets = 1; octets <= 5; ++octets)
        {
            if ((octet & mask) == mark)
                break;

            mark = (mark >> 1) | 0x80;
            mask = (mask >> 1) | 0x80;
        }

        return std::make_pair(octets, octet & ~mask);
    }

private:
    void next() { std::tie(mVal, mNxt) = decode(mNxt, mEnd); }

    Point mCur;
    Point mNxt;
    Point mEnd;
    UnicodeChar mVal;
};

#endif
