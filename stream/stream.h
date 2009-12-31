#ifndef MANGLE_STREAM_INPUT_H
#define MANGLE_STREAM_INPUT_H

#include <stdlib.h>
#include "../tools/shared_ptr.h"

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

  /// Virtual destructor
  virtual ~Stream() {}

  /** Read a given number of bytes from the stream. Returns the actual
      number read. If the return value is less than count, then the
      stream is empty or an error occured.
  */
  virtual size_t read(void* buf, size_t count) = 0;

  /// Seek to an absolute position in this stream. Not all streams are
  /// seekable.
  virtual void seek(size_t pos) = 0;

  /// Get the current position in the stream. Non-seekable streams are
  /// not required to keep track of this.
  virtual size_t tell() const = 0;

  /// Return the total size of the stream. For streams where this is
  /// not applicable, size() should return zero.
  virtual size_t size() const = 0;

  /// Returns true if the stream is empty
  virtual bool eof() const = 0;
};

typedef boost::shared_ptr<Stream> StreamPtr;

}} // namespaces
#endif
