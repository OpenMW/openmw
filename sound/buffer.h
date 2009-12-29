#ifndef MANGLE_SOUND_BUFFER_H
#define MANGLE_SOUND_BUFFER_H

#include "source.h"
#include "sources/memsource.h"
#include <vector>
#include <assert.h>

namespace Mangle {
namespace Sound {

/** A sample buffer is a factory that creates SampleSources from one
    single sound source. It is helpful when you have many instances of
    one sound and want to use one shared memory buffer.

    This is just a helper class - you don't have to include it in your
    program if you don't need it.
*/
class SampleBuffer
{
  std::vector<uint8_t> buffer;

 public:
  /// Reads the source into a memory buffer. Not heavily optimized.
  SampleBuffer(SampleSource *source)
    {
      size_t final = 0;

      while(!source->eof())
        {
          const int add = 16*1024;

          // Allocate more memory
          size_t newSize = final + add;
          buffer.resize(newSize);

          // Fill in data
          size_t read = source->read(&buffer[final], add);

          // If we couldn't read enough data, we should be at the end
          // of the stream
          assert(read == add || source->eof());

          final += read;
        }

      // Downsize the buffer to the actual length
      buffer.resize(final);
    }

  /// Get a new source
  SampleSource *get()
  { return new MemorySource(&buffer[0], buffer.size()); }
};

}}
#endif
