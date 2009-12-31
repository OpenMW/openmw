#ifndef MANGLE_STREAM_FILESERVER_H
#define MANGLE_STREAM_FILESERVER_H

#include "../stream.h"
#include <iostream>

namespace Mangle {
namespace Stream {

/** Simplest wrapper for std::istream.

    TODO: No error checking yet.
 */
class StdStream : public Stream
{
  std::istream *inf;

 public:
 StdStream(std::istream *_inf)
   : inf(_inf)
  {
    isSeekable = true;
    hasPosition = true;
    hasSize = true;
  }

  size_t read(void* buf, size_t len)
  {
    inf->read((char*)buf, len);
    return inf->gcount();
  }

  void seek(size_t pos)
  { inf->seekg(pos); }

  size_t tell() const
  // Hack around the fact that ifstream->tellg() isn't const
  { return ((StdStream*)this)->inf->tellg(); }

  size_t size() const
  {
    // Use the standard iostream size hack, terrible as it is.
    std::streampos pos = inf->tellg();
    inf->seekg(0, ios_base::end);
    size_t res = inf->tellg();
    inf->seekg(pos);
    return res;
  }

  bool eof() const
  { return inf->eof(); }
};

}} // namespaces
#endif
