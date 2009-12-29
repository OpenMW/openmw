#ifndef MANGLE_STREAM_BUFFER_H
#define MANGLE_STREAM_BUFFER_H

#include "../servers/memory_stream.h"
#include "../clients/iwrapper.h"

namespace Mangle {
namespace Stream {

/** A Stream that reads another Stream into a buffer, and serves it as
    a MemoryStream.
 */
class BufferStream : public MemoryStream
{
 public:
  BufferStream(Stream *input)
    {
      // Allocate memory, read the stream into it. Then call set()
    }
};

}} // namespaces
#endif
