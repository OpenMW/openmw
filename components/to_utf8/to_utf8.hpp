#ifndef COMPONENTS_TOUTF8_H
#define COMPONENTS_TOUTF8_H

#include <string>

namespace ToUTF8
{
  // These are all the currently supported code pages
  enum FromType
    {
      WINDOWS_1250,      // Central ane Eastern European languages
      WINDOWS_1251,      // Cyrillic languages
      WINDOWS_1252       // Used by English version of Morrowind (and
                         // probably others)
    };

  // Return a writable buffer of at least 'size' bytes. The buffer
  // does not have to be freed.
  char* getBuffer(int size);

  // Convert the previously written buffer to UTF8 from the given code
  // page.
  std::string getUtf8(FromType from);
  std::string getLegacyEnc(FromType to);
}

#endif
