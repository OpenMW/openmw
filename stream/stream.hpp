#ifndef MANGLE_STREAM_INPUT_H
#define MANGLE_STREAM_INPUT_H

#include <stdlib.h>
#include "../tools/shared_ptr.hpp"
#include <assert.h>

namespace Mangle {
namespace Stream {

/// An abstract interface for a stream data.
class Stream
{
 public:
  // Feature options. These should be set in the constructor.

  /// If true, seek() works
  bool isSeekable;

  /// If true, tell() works
  bool hasPosition;

  /// If true, size() works
  bool hasSize;

  /// If true, write() works. Writing through pointer operations is
  /// not (yet) supported.
  bool isWritable;

  /// If true, read() and eof() works.
  bool isReadable;

  /// If true, the getPtr() functions work
  bool hasPtr;

  /// Initialize all bools to false by default, except isReadable.
  Stream() :
    isSeekable(false), hasPosition(false), hasSize(false),
    isWritable(false), isReadable(true), hasPtr(false) {}

  /// Virtual destructor
  virtual ~Stream() {}

  /** Read a given number of bytes from the stream. Returns the actual
      number read. If the return value is less than count, then the
      stream is empty or an error occured. Only required for readable
      streams.
  */
  virtual size_t read(void* buf, size_t count) { assert(0); return 0; }

  /** Write a given number of bytes from the stream. Semantics is
      similar to read(). Only valid if isWritable is true.

      The returned value is the number of bytes written. However in
      most cases, unlike for read(), a write-count less than requested
      usually indicates an error. The implementation should throw such
      errors as exceptions rather than expect the caller to handle
      them.

      Since most implementations do NOT support writing we default to
      an assert(0) here.
  */
  virtual size_t write(const void *buf, size_t count) { assert(0); return 0; }

  /// Flush an output stream. Does nothing for non-writing streams.
  virtual void flush() {}

  /// Seek to an absolute position in this stream. Not all streams are
  /// seekable.
  virtual void seek(size_t pos) { assert(0); }

  /// Get the current position in the stream. Non-seekable streams are
  /// not required to keep track of this.
  virtual size_t tell() const { assert(0); return 0; }

  /// Return the total size of the stream. For streams hasSize is
  /// false, size() should fail in some way, since it is an error to
  /// call it in those cases.
  virtual size_t size() const { assert(0); return 0; }

  /// Returns true if the stream is empty. Required for readable
  /// streams.
  virtual bool eof() const { assert(0); return 0; }

  /// Return a pointer to the entire stream. This function (and the
  /// other getPtr() variants below) should only be implemented for
  /// memory-based streams where using them would be an optimization.
  virtual const void *getPtr() { assert(0); return NULL; }

  /// Get a pointer to a memory region of 'size' bytes starting from
  /// position 'pos'
  virtual const void *getPtr(size_t pos, size_t size) { assert(0); return NULL; }

  /// Get a pointer to a memory region of 'size' bytes from the
  /// current position. Unlike the two other getPtr variants, this
  /// will advance the position past the returned area.
  virtual const void *getPtr(size_t size) { assert(0); return NULL; }
};

typedef boost::shared_ptr<Stream> StreamPtr;

}} // namespaces
#endif
