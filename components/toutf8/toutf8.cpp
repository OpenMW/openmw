#include "toutf8.hpp"

#include <algorithm>
#include <cassert>
#include <ios>
#include <iterator>
#include <stdexcept>

#include <components/debug/debuglog.hpp>

/* This file contains the code to translate from WINDOWS-1252 (native
   charset used in English version of Morrowind) to UTF-8. The library
   is designed to be extened to support more source encodings later,
   which means that we may add support for Russian, Polish and Chinese
   files and so on.

   The code does not depend on any external library at
   runtime. Instead, it uses a pregenerated table made with iconv (see
   gen_iconv.cpp and the Makefile) which is located in tables_gen.hpp.

   This is both faster and uses less dependencies. The tables would
   only need to be regenerated if we are adding support more input
   encodings. As such, there is no need to make the generator code
   platform independent.

   The library is optimized for the case of pure ASCII input strings,
   which is the vast majority of cases at least for the English
   version. A test of my version of Morrowind.esm got 130 non-ASCII vs
   236195 ASCII strings, or less than 0.06% of strings containing
   non-ASCII characters.

   To optmize for this, ff the first pass of the string does not find
   any non-ASCII characters, the entire string is passed along without
   any modification.

   Most of the non-ASCII strings are books, and are quite large. (The
   non-ASCII characters are typically starting and ending quotation
   marks.) Within these, almost all the characters are ASCII. For this
   purpose, the library is also optimized for mostly-ASCII contents
   even in the cases where some conversion is necessary.
 */

// Generated tables
#include "tablesgen.hpp"

using namespace ToUTF8;

namespace
{
    std::string_view::iterator skipAscii(std::string_view input)
    {
        return std::find_if(input.begin(), input.end(), [](unsigned char v) { return v == 0 || v >= 128; });
    }

    std::basic_string_view<signed char> getTranslationArray(FromType sourceEncoding)
    {
        switch (sourceEncoding)
        {
            case ToUTF8::WINDOWS_1252:
                return { ToUTF8::windows_1252, std::size(ToUTF8::windows_1252) };
            case ToUTF8::WINDOWS_1250:
                return { ToUTF8::windows_1250, std::size(ToUTF8::windows_1250) };
            case ToUTF8::WINDOWS_1251:
                return { ToUTF8::windows_1251, std::size(ToUTF8::windows_1251) };
            case ToUTF8::CP437:
                return { ToUTF8::cp437, std::size(ToUTF8::cp437) };
        }
        throw std::logic_error("Invalid source encoding: " + std::to_string(sourceEncoding));
    }

    // Make sure the output vector is large enough for 'size' bytes,
    // including a terminating zero after it.
    void resize(std::size_t size, BufferAllocationPolicy bufferAllocationPolicy, std::string& buffer)
    {
        if (buffer.size() > size)
        {
            buffer[size] = 0;
            return;
        }

        if (buffer.size() == size)
            return;

        switch (bufferAllocationPolicy)
        {
            case BufferAllocationPolicy::FitToRequiredSize:
                buffer.resize(size);
                break;
            case BufferAllocationPolicy::UseGrowFactor:
                // Add some extra padding to reduce the chance of having to resize
                // again later.
                buffer.resize(3 * size);
                // And make sure the string is zero terminated
                buffer[size] = 0;
                break;
        }
    }
}

StatelessUtf8Encoder::StatelessUtf8Encoder(FromType sourceEncoding)
    : mTranslationArray(getTranslationArray(sourceEncoding))
{
}

std::string_view StatelessUtf8Encoder::getUtf8(
    std::string_view input, BufferAllocationPolicy bufferAllocationPolicy, std::string& buffer) const
{
    if (input.empty())
        return input;

    // Note: The rest of this function is designed for single-character
    // input encodings only. It also assumes that the input encoding
    // shares its first 128 values (0-127) with ASCII. There are no plans
    // to add more encodings to this module (we are using utf8 for new
    // content files), so that shouldn't be an issue.

    // Compute output length, and check for pure ascii input at the same
    // time.
    const auto [outlen, ascii] = getLength(input);

    // If we're pure ascii, then don't bother converting anything.
    if (ascii)
        return std::string_view(input.data(), outlen);

    // Make sure the output is large enough
    resize(outlen, bufferAllocationPolicy, buffer);
    char* out = buffer.data();

    // Translate
    for (auto it = input.begin(); it != input.end() && *it != 0; ++it)
        copyFromArray(*it, out);

    // Make sure that we wrote the correct number of bytes
    assert((out - buffer.data()) == (int)outlen);

    // And make extra sure the output is null terminated
    assert(buffer.size() >= outlen);
    assert(buffer[outlen] == 0);

    return std::string_view(buffer.data(), outlen);
}

std::string_view StatelessUtf8Encoder::getLegacyEnc(
    std::string_view input, BufferAllocationPolicy bufferAllocationPolicy, std::string& buffer) const
{
    if (input.empty())
        return input;

    // TODO: The rest of this function is designed for single-character
    // input encodings only. It also assumes that the input the input
    // encoding shares its first 128 values (0-127) with ASCII. These
    // conditions must be checked again if you add more input encodings
    // later.

    // Compute output length, and check for pure ascii input at the same
    // time.
    const auto [outlen, ascii] = getLengthLegacyEnc(input);

    // If we're pure ascii, then don't bother converting anything.
    if (ascii)
        return std::string_view(input.data(), outlen);

    // Make sure the output is large enough
    resize(outlen, bufferAllocationPolicy, buffer);
    char* out = buffer.data();

    // Translate
    for (auto it = input.begin(); it != input.end() && *it != 0;)
        copyFromArrayLegacyEnc(it, input.end(), out);

    // Make sure that we wrote the correct number of bytes
    assert((out - buffer.data()) == static_cast<int>(outlen));

    // And make extra sure the output is null terminated
    assert(buffer.size() >= outlen);
    assert(buffer[outlen] == 0);

    return std::string_view(buffer.data(), outlen);
}

/** Get the total length length needed to decode the given string with
  the given translation array. The arrays are encoded with 6 bytes
  per character, with the first giving the length and the next 5 the
  actual data.

  The function serves a dual purpose for optimization reasons: it
  checks if the input is pure ascii (all values are <= 127). If this
  is the case, then the ascii parameter is set to true, and the
  caller can optimize for this case.
 */
std::pair<std::size_t, bool> StatelessUtf8Encoder::getLength(std::string_view input) const
{
    // Do away with the ascii part of the string first (this is almost
    // always the entire string.)
    auto it = skipAscii(input);

    // If we're not at the null terminator at this point, then there
    // were some non-ascii characters to deal with. Go to slow-mode for
    // the rest of the string.
    if (it == input.end() || *it == 0)
        return { it - input.begin(), true };

    std::size_t len = it - input.begin();

    do
    {
        // Find the translated length of this character in the
        // lookup table.
        len += mTranslationArray[static_cast<unsigned char>(*it) * 6];
        ++it;
    } while (it != input.end() && *it != 0);

    return { len, false };
}

// Translate one character 'ch' using the translation array 'arr', and
// advance the output pointer accordingly.
void StatelessUtf8Encoder::copyFromArray(unsigned char ch, char*& out) const
{
    // Optimize for ASCII values
    if (ch < 128)
    {
        *(out++) = ch;
        return;
    }

    const signed char* in = &mTranslationArray[ch * 6];
    int len = *(in++);
    memcpy(out, in, len);
    out += len;
}

std::pair<std::size_t, bool> StatelessUtf8Encoder::getLengthLegacyEnc(std::string_view input) const
{
    // Do away with the ascii part of the string first (this is almost
    // always the entire string.)
    auto it = skipAscii(input);

    // If we're not at the null terminator at this point, then there
    // were some non-ascii characters to deal with. Go to slow-mode for
    // the rest of the string.
    if (it == input.end() || *it == 0)
        return { it - input.begin(), true };

    std::size_t len = it - input.begin();
    std::size_t symbolLen = 0;

    do
    {
        symbolLen += 1;
        // Find the translated length of this character in the
        // lookup table.
        switch (static_cast<unsigned char>(*it))
        {
            case 0xe2:
                symbolLen -= 2;
                break;
            case 0xc2:
            case 0xcb:
            case 0xc4:
            case 0xc6:
            case 0xc3:
            case 0xd0:
            case 0xd1:
            case 0xd2:
            case 0xc5:
                symbolLen -= 1;
                break;
            default:
                len += symbolLen;
                symbolLen = 0;
                break;
        }

        ++it;
    } while (it != input.end() && *it != 0);

    return { len, false };
}

void StatelessUtf8Encoder::copyFromArrayLegacyEnc(
    std::string_view::iterator& chp, std::string_view::iterator end, char*& out) const
{
    unsigned char ch = *(chp++);
    // Optimize for ASCII values
    if (ch < 128)
    {
        *(out++) = ch;
        return;
    }

    int len = 1;
    switch (ch)
    {
        case 0xe2:
            len = 3;
            break;
        case 0xc2:
        case 0xcb:
        case 0xc4:
        case 0xc6:
        case 0xc3:
        case 0xd0:
        case 0xd1:
        case 0xd2:
        case 0xc5:
            len = 2;
            break;
    }

    if (len == 1) // There is no 1 length utf-8 glyph that is not 0x20 (empty space)
    {
        *(out++) = ch;
        return;
    }

    if (chp == end)
        return;

    unsigned char ch2 = *(chp++);
    unsigned char ch3 = '\0';
    if (len == 3)
    {
        if (chp == end)
            return;
        ch3 = *(chp++);
    }

    for (int i = 128; i < 256; i++)
    {
        unsigned char b1 = mTranslationArray[i * 6 + 1], b2 = mTranslationArray[i * 6 + 2],
                      b3 = mTranslationArray[i * 6 + 3];
        if (b1 == ch && b2 == ch2 && (len != 3 || b3 == ch3))
        {
            *(out++) = (char)i;
            return;
        }
    }

    Log(Debug::Info) << "Could not find glyph " << std::hex << (int)ch << " " << (int)ch2 << " " << (int)ch3;

    *(out++) = ch; // Could not find glyph, just put whatever
}

Utf8Encoder::Utf8Encoder(FromType sourceEncoding)
    : mBuffer(50 * 1024, '\0')
    , mImpl(sourceEncoding)
{
}

std::string_view Utf8Encoder::getUtf8(std::string_view input)
{
    return mImpl.getUtf8(input, BufferAllocationPolicy::UseGrowFactor, mBuffer);
}

std::string_view Utf8Encoder::getLegacyEnc(std::string_view input)
{
    return mImpl.getLegacyEnc(input, BufferAllocationPolicy::UseGrowFactor, mBuffer);
}

ToUTF8::FromType ToUTF8::calculateEncoding(std::string_view encodingName)
{
    if (encodingName == "win1250")
        return ToUTF8::WINDOWS_1250;
    else if (encodingName == "win1251")
        return ToUTF8::WINDOWS_1251;
    else if (encodingName == "win1252")
        return ToUTF8::WINDOWS_1252;
    else
        throw std::runtime_error(
            "Unknown encoding '" + std::string(encodingName) + "', see openmw --help for available options.");
}

std::string ToUTF8::encodingUsingMessage(std::string_view encodingName)
{
    if (encodingName == "win1250")
        return "Using Central and Eastern European font encoding.";
    else if (encodingName == "win1251")
        return "Using Cyrillic font encoding.";
    else if (encodingName == "win1252")
        return "Using default (English) font encoding.";
    else
        throw std::runtime_error(
            "Unknown encoding '" + std::string(encodingName) + "', see openmw --help for available options.");
}
