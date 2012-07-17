#include "to_utf8.hpp"

#include <vector>
#include <cassert>

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

// Shared global buffers, we love you. These initial sizes are large
// enough to hold the largest books in Morrowind.esm, but we will
// resize automaticall if necessary.
static std::vector<char> buf    (50*1024);
static std::vector<char> output (50*1024);
static int size;

// Make sure the given vector is large enough for 'size' bytes,
// including a terminating zero after it.
static void resize(std::vector<char> &buf, size_t size)
{
  if(buf.size() <= size)
    // Add some extra padding to reduce the chance of having to resize
    // again later.
    buf.resize(3*size);

  // And make sure the string is zero terminated
  buf[size] = 0;
}

// This is just used to spew out a reusable input buffer for the
// conversion process.
char *ToUTF8::getBuffer(int s)
{
  // Remember the requested size
  size = s;
  resize(buf, size);
  return &buf[0];
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
static size_t getLength(const char *arr, const char* input, bool &ascii)
{
  ascii = true;
  size_t len = 0;
  const char* ptr = input;
  unsigned char inp = *ptr;

  // Do away with the ascii part of the string first (this is almost
  // always the entire string.)
  while(inp && inp < 128)
    inp = *(++ptr);
  len += (ptr-input);

  // If we're not at the null terminator at this point, then there
  // were some non-ascii characters to deal with. Go to slow-mode for
  // the rest of the string.
  if(inp)
    {
      ascii = false;
      while(inp)
        {
          // Find the translated length of this character in the
          // lookup table.
          len += arr[inp*6];
          inp = *(++ptr);
        }
    }
  return len;
}

// Translate one character 'ch' using the translation array 'arr', and
// advance the output pointer accordingly.
static void copyFromArray(const char *arr, unsigned char ch, char* &out)
{
  // Optimize for ASCII values
  if(ch < 128)
    {
      *(out++) = ch;
      return;
    }

  const char *in = arr + ch*6;
  int len = *(in++);
  for(int i=0; i<len; i++)
    *(out++) = *(in++);
}

std::string ToUTF8::getUtf8(ToUTF8::FromType from)
{
  // Pick translation array
  const char *arr;
  switch (from)
  {
    case ToUTF8::WINDOWS_1252:
    {
      arr = ToUTF8::windows_1252;
      break;
    }
    case ToUTF8::WINDOWS_1250:
    {
      arr = ToUTF8::windows_1250;
      break;
    }
    case ToUTF8::WINDOWS_1251:
    {
      arr = ToUTF8::windows_1251;
      break;
    }
    default:
    {
      assert(0);
    }
  }

  // Double check that the input string stops at some point (it might
  // contain zero terminators before this, inside its own data, which
  // is also ok.)
  const char* input = &buf[0];
  assert(input[size] == 0);

  // TODO: The rest of this function is designed for single-character
  // input encodings only. It also assumes that the input the input
  // encoding shares its first 128 values (0-127) with ASCII. These
  // conditions must be checked again if you add more input encodings
  // later.

  // Compute output length, and check for pure ascii input at the same
  // time.
  bool ascii;
  size_t outlen = getLength(arr, input, ascii);

  // If we're pure ascii, then don't bother converting anything.
  if(ascii)
    return std::string(input, outlen);

  // Make sure the output is large enough
  resize(output, outlen);
  char *out = &output[0];

  // Translate
  while(*input)
    copyFromArray(arr, *(input++), out);

  // Make sure that we wrote the correct number of bytes
  assert((out-&output[0]) == (int)outlen);

  // And make extra sure the output is null terminated
  assert(output.size() > outlen);
  assert(output[outlen] == 0);

  // Return a string
  return std::string(&output[0], outlen);
}

