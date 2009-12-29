#ifndef MANGLE_STREAM_MEMSERVER_H
#define MANGLE_STREAM_MEMSERVER_H

#include <assert.h>
#include "../stream.h"

namespace Mangle {
namespace Stream {

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

  size_t read(void *buf, size_t count)
  {
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

  // New members in MemoryStream:

  /// Get the base pointer to the entire buffer
  const void *getPtr() const { return data; }

  /// Clone this memory stream
  /** Make a new stream of the same buffer. If setPos is true, we also
      set the clone's position to be the same as ours.

      No memory is copied during this operation, the new stream is
      just another 'view' into the same shared memory buffer.
  */
  MemoryStream *clone(bool setPos=false) const
  {
    MemoryStream *res = new MemoryStream(data, length);
    if(setPos) res->seek(pos);
    return res;
  }
};

}} // namespaces
#endif
