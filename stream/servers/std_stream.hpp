#ifndef MANGLE_STREAM_STDIOSERVER_H
#define MANGLE_STREAM_STDIOSERVER_H

#include "../stream.hpp"
#include <iostream>
#include <stdexcept>

namespace Mangle {
namespace Stream {

/** Simple wrapper for std::istream.
 */
class StdStream : public Stream
{
  std::istream *inf;

  static void fail(const std::string &msg)
  { throw std::runtime_error("StdStream: " + msg); }

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
    if(inf->bad())
      fail("error reading from stream");
    return inf->gcount();
  }

  void seek(size_t pos)
  {
    inf->clear();
    inf->seekg(pos);
    if(inf->fail())
      fail("seek error");
  }

  size_t tell() const
  // Hack around the fact that ifstream->tellg() isn't const
  { return ((StdStream*)this)->inf->tellg(); }

  size_t size() const
  {
    // Use the standard iostream size hack, terrible as it is.
    std::streampos pos = inf->tellg();
    inf->seekg(0, std::ios::end);
    size_t res = inf->tellg();
    inf->seekg(pos);

    if(inf->fail())
      fail("could not get stream size");

    return res;
  }

  bool eof() const
  { return inf->eof(); }
};

typedef boost::shared_ptr<StdStream> StdStreamPtr;

}} // namespaces
#endif
