#ifndef MANGLE_STREAM_BUFFER_H
#define MANGLE_STREAM_BUFFER_H

#include "../servers/memory_stream.h"
#include <vector>

namespace Mangle {
namespace Stream {

/** A Stream that reads another Stream into a buffer, and serves it as
    a MemoryStream. Might be expanded with other capabilities later.
 */

class BufferStream : public MemoryStream
{
  std::vector<char> buffer;

 public:
  BufferStream(StreamPtr input)
    {
      assert(input);

      // Allocate memory, read the stream into it. Then call set()
      if(input->hasSize)
        {
          // We assume that we can get the position as well
          assert(input->hasPosition);

          // Calculate how much is left of the stream
          size_t left = input->size() - input->tell();

          // Allocate the buffer and fill it
          buffer.resize(left);
          input->read(&buffer[0], left);
        }
      else
        {
          // We DON'T know how big the stream is. We'll have to read
          // it in increments.
          const int ADD = 32*1024;
          size_t len=0, newlen;

          while(!input->eof())
            {
              // Read one block
              newlen = len + ADD;
              buffer.resize(newlen);
              size_t read = input->read(&buffer[len], ADD);

              // Increase the total length
              len += read;

              // If we read less than expected, we should be at the
              // end of the stream
              assert(read == ADD || input->eof());
            }

          // Downsize to match the real length
          buffer.resize(len);
        }

      // After the buffer has been filled, set up our MemoryStream
      // ancestor to reference it.
      set(&buffer[0], buffer.size());
    }
};

typedef boost::shared_ptr<BufferStream> BufferStreamPtr;

}} // namespaces
#endif
