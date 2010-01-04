#ifndef MANGLE_STREAM_MEMSERVER_H
#define MANGLE_STREAM_MEMSERVER_H

#include <assert.h>
#include "../stream.h"
#include <string.h>

namespace Mangle {
namespace Stream {

// Do this before the class declaration, since the class itself
// depends on it.
class MemoryStream;
typedef boost::shared_ptr<MemoryStream> MemoryStreamPtr;

/** A Stream wrapping a memory buffer

    This will create a fully seekable stream out any pointer/length
    pair you give it.
 */
class MemoryStream : public Stream
{
  const void *data;
  size_t length, pos;

 public:
 MemoryStream(const void *ptr, size_t len)
   : data(ptr), length(len), pos(0)
  {
    isSeekable = true;
    hasPosition = true;
    hasSize = true;
  }

  MemoryStream()
    : data(NULL), length(0), pos(0)
  {
    isSeekable = true;
    hasPosition = true;
    hasSize = true;
    hasPtr = true;
  }

  size_t read(void *buf, size_t count)
  {
    assert(data != NULL);
    assert(pos <= length);

    // Don't read more than we have
    if(count > (length - pos))
      count = length - pos;

    // Copy data
    if(count)
      memcpy(buf, ((char*)data)+pos, count);

    // aaand remember to increase the count
    pos += count;

    return count;
  }

  void seek(size_t _pos)
  {
    pos = _pos;
    if(pos > length)
      pos = length;
  }

  size_t tell() const { return pos; }
  size_t size() const { return length; }
  bool eof() const { return pos == length; }

  const void *getPtr() { return data; }
  const void *getPtr(size_t size)
    {
      // This variant of getPtr must move the position pointer
      size_t opos = pos;
      pos += size;
      if(pos > length) pos = length;
      return ((char*)data)+opos;
    }
  const void *getPtr(size_t pos, size_t size)
    {
      if(pos > length) pos = length;
      return ((char*)data)+pos;
    }

  // New members in MemoryStream:

  /// Set a new buffer and length. This will rewind the position to zero.
  void set(const void* _data, size_t _length)
  {
    data = _data;
    length = _length;
    pos = 0;
  }

  /// Clone this memory stream
  /** Make a new stream of the same buffer. If setPos is true, we also
      set the clone's position to be the same as ours.

      No memory is copied during this operation, the new stream is
      just another 'view' into the same shared memory buffer.
  */
  MemoryStreamPtr clone(bool setPos=false) const
  {
    MemoryStreamPtr res(new MemoryStream(data, length));
    if(setPos) res->seek(pos);
    return res;
  }
};

}} // namespaces
#endif
