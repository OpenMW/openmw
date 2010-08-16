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

  // Move the remains from the last operation first
  if(pullSize)
    {
      // pullSize is how much was stored the last time. The data is
      // stored at the end of the buffer.
      memcpy(data, pullOver+(frameSize-pullSize), pullSize);
      length -= pullSize;
      data += pullSize;
      pullSize = 0;
    }

  // Number of whole frames
  size_t frames = length / frameSize;

  // Read the data
  size_t res = readSamples(data, frames);

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

  // Are we missing data?
  if(pullSize)
    {
      // Fill in one sample
      res = readSamples(pullOver,1);
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
