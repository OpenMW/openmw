#include "sample_reader.hpp"

#include <string.h>

using namespace Mangle::Sound;

void SampleReader::setup(int size)
{
  pullSize = 0;
  frameSize = size;
  pullOver = new char[size];
}

SampleReader::~SampleReader()
{
  if(pullOver)
    delete[] pullOver;
}

size_t SampleReader::read(void *_data, size_t length)
{
  if(isEof) return 0;
  char *data = (char*)_data;

  // Pullsize holds the number of bytes that were copied "extra" at
  // the end of LAST round. If non-zero, it also means there is data
  // left in the pullOver buffer.
  if(pullSize)
    {
      // Amount of data left
      size_t doRead = frameSize - pullSize;
      assert(doRead > 0);

      // Make sure we don't read more than we're supposed to
      if(doRead > length) doRead = length;

      memcpy(data, pullOver+pullSize, doRead);

      // Update the number of bytes now copied
      pullSize += doRead;
      assert(pullSize <= frameSize);

      if(pullSize < frameSize)
        {
          // There is STILL data left in the pull buffer, and we've
          // done everything we were supposed to. Leave it and return.
          assert(doRead == length);
          return doRead;
        }

      // Set up variables for further reading below. No need to update
      // pullSize, it is overwritten anyway.
      length -= doRead;
      data += doRead;
    }

  // Number of whole frames
  size_t frames = length / frameSize;

  // Read the data
  size_t res = readSamples(data, frames);
  assert(res <= frames);

  // Total bytes read
  size_t num = res*frameSize;
  data += num;

  if(res < frames)
    {
      // End of stream.
      isEof = true;
      // Determine how much we read
      return data-(char*)_data;
    }
  
  // Determine the overshoot
  pullSize = length - num;
  assert(pullSize < frameSize && pullSize >= 0);

  // Are we missing data?
  if(pullSize)
    {
      // Fill in one sample
      res = readSamples(pullOver,1);
      assert(res == 1 || res == 0);
      if(res)
        {
          // Move as much as we can into the output buffer
          memcpy(data, pullOver, pullSize);
          data += pullSize;
        }
      else
        // Failed reading, we're out of data
        isEof = true;
    }

  // Return the total number of bytes stored
  return data-(char*)_data;
}
