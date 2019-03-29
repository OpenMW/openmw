#include "to_utf8.hpp"

#include <vector>
#include <cassert>
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
#include "tables_gen.hpp"

using namespace ToUTF8;

Utf8Encoder::Utf8Encoder(const FromType sourceEncoding):
    mOutput(50*1024)
{
    switch (sourceEncoding)
    {
        case ToUTF8::WINDOWS_1252:
        {
            translationArray = ToUTF8::windows_1252;
            break;
        }
        case ToUTF8::WINDOWS_1250:
        {
            translationArray = ToUTF8::windows_1250;
            break;
        }
        case ToUTF8::WINDOWS_1251:
        {
            translationArray = ToUTF8::windows_1251;
            break;
        }
        case ToUTF8::CP437:
        {
            translationArray = ToUTF8::cp437;
            break;
        }

        default:
        {
            assert(0);
        }
    }
}

std::string Utf8Encoder::getUtf8(const char* input, size_t size)
{
    // Double check that the input string stops at some point (it might
    // contain zero terminators before this, inside its own data, which
    // is also ok.)
    assert(input[size] == 0);

    // Note: The rest of this function is designed for single-character
    // input encodings only. It also assumes that the input encoding
    // shares its first 128 values (0-127) with ASCII. There are no plans
    // to add more encodings to this module (we are using utf8 for new
    // content files), so that shouldn't be an issue.

    // Compute output length, and check for pure ascii input at the same
    // time.
    bool ascii;
    size_t outlen = getLength(input, ascii);

    // If we're pure ascii, then don't bother converting anything.
    if(ascii)
        return std::string(input, outlen);

    // Make sure the output is large enough
    resize(outlen);
    char *out = &mOutput[0];

    // Translate
    while (*input)
        copyFromArray(*(input++), out);

    // Make sure that we wrote the correct number of bytes
    assert((out-&mOutput[0]) == (int)outlen);

    // And make extra sure the output is null terminated
    assert(mOutput.size() > outlen);
    assert(mOutput[outlen] == 0);

    // Return a string
    return std::string(&mOutput[0], outlen);
}

std::string Utf8Encoder::getLegacyEnc(const char *input, size_t size)
{
    // Double check that the input string stops at some point (it might
    // contain zero terminators before this, inside its own data, which
    // is also ok.)
    assert(input[size] == 0);

    // TODO: The rest of this function is designed for single-character
    // input encodings only. It also assumes that the input the input
    // encoding shares its first 128 values (0-127) with ASCII. These
    // conditions must be checked again if you add more input encodings
    // later.

    // Compute output length, and check for pure ascii input at the same
    // time.
    bool ascii;
    size_t outlen = getLength2(input, ascii);

    // If we're pure ascii, then don't bother converting anything.
    if(ascii)
        return std::string(input, outlen);

    // Make sure the output is large enough
    resize(outlen);
    char *out = &mOutput[0];

    // Translate
    while(*input)
        copyFromArray2(input, out);

    // Make sure that we wrote the correct number of bytes
    assert((out-&mOutput[0]) == (int)outlen);

    // And make extra sure the output is null terminated
    assert(mOutput.size() > outlen);
    assert(mOutput[outlen] == 0);

    // Return a string
    return std::string(&mOutput[0], outlen);
}

// Make sure the output vector is large enough for 'size' bytes,
// including a terminating zero after it.
void Utf8Encoder::resize(size_t size)
{
    if (mOutput.size() <= size)
        // Add some extra padding to reduce the chance of having to resize
        // again later.
        mOutput.resize(3*size);

    // And make sure the string is zero terminated
    mOutput[size] = 0;
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
size_t Utf8Encoder::getLength(const char* input, bool &ascii)
{
    ascii = true;
    size_t len = 0;
    const char* ptr = input;
    unsigned char inp = *ptr;

    // Do away with the ascii part of the string first (this is almost
    // always the entire string.)
    while (inp && inp < 128)
        inp = *(++ptr);
    len += (ptr-input);

    // If we're not at the null terminator at this point, then there
    // were some non-ascii characters to deal with. Go to slow-mode for
    // the rest of the string.
    if (inp)
    {
        ascii = false;
        while (inp)
        {
            // Find the translated length of this character in the
            // lookup table.
            len += translationArray[inp*6];
            inp = *(++ptr);
        }
    }
    return len;
}

// Translate one character 'ch' using the translation array 'arr', and
// advance the output pointer accordingly.
void Utf8Encoder::copyFromArray(unsigned char ch, char* &out)
{
    // Optimize for ASCII values
    if (ch < 128)
    {
        *(out++) = ch;
        return;
    }

    const signed char *in = translationArray + ch*6;
    int len = *(in++);
    for (int i=0; i<len; i++)
        *(out++) = *(in++);
}

size_t Utf8Encoder::getLength2(const char* input, bool &ascii)
{
    ascii = true;
    size_t len = 0;
    const char* ptr = input;
    unsigned char inp = *ptr;

    // Do away with the ascii part of the string first (this is almost
    // always the entire string.)
    while (inp && inp < 128)
        inp = *(++ptr);
    len += (ptr-input);

    // If we're not at the null terminator at this point, then there
    // were some non-ascii characters to deal with. Go to slow-mode for
    // the rest of the string.
    if (inp)
    {
        ascii = false;
        while(inp)
        {
            len += 1;
            // Find the translated length of this character in the
            // lookup table.
            switch(inp)
            {
                case 0xe2: len -= 2; break;
                case 0xc2:
                case 0xcb:
                case 0xc4:
                case 0xc6:
                case 0xc3:
                case 0xd0:
                case 0xd1:
                case 0xd2:
                case 0xc5: len -= 1; break;
            }

            inp = *(++ptr);
        }
    }
    return len;
}

void Utf8Encoder::copyFromArray2(const char*& chp, char* &out)
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
        case 0xe2: len = 3; break;
        case 0xc2:
        case 0xcb:
        case 0xc4:
        case 0xc6:
        case 0xc3:
        case 0xd0:
        case 0xd1:
        case 0xd2:
        case 0xc5: len = 2; break;
    }

    if (len == 1) // There is no 1 length utf-8 glyph that is not 0x20 (empty space)
    {
        *(out++) = ch;
        return;
    }

    unsigned char ch2 = *(chp++);
    unsigned char ch3 = '\0';
    if (len == 3)
        ch3 = *(chp++);

    for (int i = 128; i < 256; i++)
    {
        unsigned char b1 = translationArray[i*6 + 1], b2 = translationArray[i*6 + 2], b3 = translationArray[i*6 + 3];
        if (b1 == ch && b2 == ch2 && (len != 3 || b3 == ch3))
        {
            *(out++) = (char)i;
            return;
        }
    }

    Log(Debug::Info) << "Could not find glyph " << std::hex << (int)ch << " " << (int)ch2 << " " << (int)ch3;

    *(out++) = ch; // Could not find glyph, just put whatever
}

ToUTF8::FromType ToUTF8::calculateEncoding(const std::string& encodingName)
{
    if (encodingName == "win1250")
        return ToUTF8::WINDOWS_1250;
    else if (encodingName == "win1251")
        return ToUTF8::WINDOWS_1251;
    else if (encodingName == "win1252")
        return ToUTF8::WINDOWS_1252;
    else
        throw std::runtime_error(std::string("Unknown encoding '") + encodingName + std::string("', see openmw --help for available options."));
}

std::string ToUTF8::encodingUsingMessage(const std::string& encodingName)
{
    if (encodingName == "win1250")
        return "Using Central and Eastern European font encoding.";
    else if (encodingName == "win1251")
        return "Using Cyrillic font encoding.";
    else if (encodingName == "win1252")
        return "Using default (English) font encoding.";
    else
        throw std::runtime_error(std::string("Unknown encoding '") + encodingName + std::string("', see openmw --help for available options."));
}
