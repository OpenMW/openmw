#ifndef MANGLE_STREAM_IOSTREAM_H
#define MANGLE_STREAM_IOSTREAM_H

#include <assert.h>
#include "../stream.hpp"
#include <iostream>

namespace Mangle {
namespace Stream {

  /** This file contains classes for wrapping an std::istream or
      std::ostream around a Mangle::Stream.

      This allows you to use Mangle streams in places that require std
      streams.

      This is much easier than trying to make your own custom streams
      into iostreams. The std::iostream interface is horrible and NOT
      designed for easy subclassing. Create a Mangle::Stream instead,
      and use this wrapper.
  */

  // An istream wrapping a readable Mangle::Stream. Has extra
  // optimizations for pointer-based streams.
  class MangleIStream : public std::istream
  {
    std::streambuf *buf;
  public:
    MangleIStream(StreamPtr inp);
    ~MangleIStream();
  };

  // An ostream wrapping a writable Mangle::Stream.
  class MangleOStream : public std::ostream
  {
    std::streambuf *buf;
  public:
    MangleOStream(StreamPtr inp);
    ~MangleOStream();
  };

}} // namespaces
#endif
