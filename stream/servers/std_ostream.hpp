#ifndef MANGLE_OSTREAM_STDIOSERVER_H
#define MANGLE_OSTREAM_STDIOSERVER_H

#include "../stream.hpp"
#include <iostream>
#include <stdexcept>

namespace Mangle {
namespace Stream {

/** Simple wrapper for std::ostream, only supports output.
 */
class StdOStream : public Stream
{
  std::ostream *inf;

  static void fail(const std::string &msg)
  { throw std::runtime_error("StdOStream: " + msg); }

 public:
 StdOStream(std::ostream *_inf)
   : inf(_inf)
  {
    isSeekable = true;
    hasPosition = true;
    hasSize = true;
    isWritable = true;
  }

  size_t read(void*,size_t)
  {
    assert(0&&"reading not supported by StdOStream");
  }

  size_t write(const void* buf, size_t len)
  {
    inf->write((const char*)buf, len);
    if(inf->fail())
      fail("error writing to stream");
    // Just return len, but that is ok. The only cases where we would
    // return less than len is when an error occured.
    return len;
  }

  void flush()
  {
    inf->flush();
  }

  void seek(size_t pos)
  {
    inf->seekp(pos);
    if(inf->fail())
      fail("seek error");
  }

  size_t tell() const
  // Hack around the fact that ifstream->tellp() isn't const
  { return ((StdOStream*)this)->inf->tellp(); }

  size_t size() const
  {
    // Use the standard iostream size hack, terrible as it is.
    std::streampos pos = inf->tellp();
    inf->seekp(0, std::ios::end);
    size_t res = inf->tellp();
    inf->seekp(pos);

    if(inf->fail())
      fail("could not get stream size");

    return res;
  }

  bool eof() const
  { return inf->eof(); }
};

typedef boost::shared_ptr<StdOStream> StdOStreamPtr;

}} // namespaces
#endif
