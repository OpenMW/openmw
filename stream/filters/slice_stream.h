#ifndef MANGLE_STREAM_SLICE_H
#define MANGLE_STREAM_SLICE_H

#include "../stream.h"

namespace Mangle {
namespace Stream {

/** A Stream that represents a subset (called a slice) of another stream.
 */
class SliceStream : public Stream
{
  StreamPtr src;
  size_t offset, length, pos;

 public:
  SliceStream(StreamPtr _src, size_t _offset, size_t _length)
    : src(_src), offset(_offset), length(_length), pos(0)
    {
      assert(src->hasSize);
      assert(src->isSeekable);

      // Make sure we can actually fit inside the source stream
      assert(src->size() <= offset+length);

      isSeekable = true;
      hasPosition = true;
      hasSize = true;
      hasPtr = src->hasPtr;
    }

  size_t read(void *buf, size_t count)
  {
    // Check that we're not reading past our slice
    if(count > length-pos)
      count = length-pos;

    // Seek into place and reading
    src->seek(offset+pos);
    count = src->read(buf, count);

    pos += count;
    assert(pos <= length);
    return count;
  }

  void seek(size_t _pos)
  {
    pos = _pos;
    if(pos > length) pos = length;
  }

  bool eof() { return pos == length; }
  size_t tell() { return pos; }
  size_t size() { return length; }

  void *getPtr() { return getPtr(0, length); }
  void *getPtr(size_t size) { return getPtr(pos, size); }
  void *getPtr(size_t pos, size_t size)
    {
      // Boundry checks on pos and size. Bounding the size is
      // important even when getting pointers, as the source stream
      // may use the size parameter for something (such as memory
      // mapping or buffering.)
      if(pos > length) pos = length;
      if(pos+size > length) size = length-pos;

      // Ask the source to kindly give us a pointer
      return src->getPtr(offset+pos, size);
    }
};

typedef boost::shared_ptr<SliceStream> SliceStreamPtr;

}} // namespaces
#endif
