#ifndef MANGLE_STREAM_FILTER_H
#define MANGLE_STREAM_FILTER_H

#include "../stream.h"

namespace Mangle {
namespace Stream {

/** A stream that filters another stream with no changes. Intended as
    a base class for other filters.
 */
class PureFilter : public Stream
{
 protected:
  StreamPtr src;

 public:
  PureFilter(StreamPtr _src)
    : src(_src)
    {
      isSeekable = src->isSeekable;
      hasPosition = src->hasPosition;
      hasSize = src->hasSize;
      hasPtr = src->hasPtr;
    }

  size_t read(void *buf, size_t count) { return src->read(buf, count); }
  void seek(size_t pos) { src->seek(pos); }
  size_t tell() const { return src->tell(); }
  size_t size() const { return src->size(); }
  bool eof() const { return src->eof(); }
  const void *getPtr() { return src->getPtr(); }
  const void *getPtr(size_t size) { return src->getPtr(size); }
  const void *getPtr(size_t pos, size_t size)
    { return src->getPtr(pos, size); }
};

}} // namespaces
#endif
