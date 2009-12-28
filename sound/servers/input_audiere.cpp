#include "input_audiere.h"
#include <assert.h>

#include "../../stream/clients/audiere_file.h"

// Exception handling
class Audiere_Exception : public std::exception
{
  std::string msg;

 public:

  Audiere_Exception(const std::string &m) : msg(m) {}
  ~Audiere_Exception() throw() {}
  virtual const char* what() const throw() { return msg.c_str(); }
};

static void fail(const std::string &msg)
{
  throw Audiere_Exception("Audiere exception: " + msg);
}

using namespace audiere;
using namespace Mangle::Sound;

// --- InputManager ---

AudiereInput::AudiereInput()
{
  canLoadStream = true;
}

InputSource *AudiereInput::load(const std::string &file)
{ return new AudiereSource(file); }

InputSource *AudiereInput::load(Stream::Stream *input)
{ return new AudiereSource(input); }

// --- InputSource ---

AudiereSource::AudiereSource(const std::string &file)
{
  SampleSourcePtr sample = OpenSampleSource(file.c_str());
  if(!sample)
    fail("Couldn't load file " + file);

  buf = CreateSampleBuffer(sample);
}

AudiereSource::AudiereSource(Stream::Stream *input)
{
  SampleSourcePtr sample = OpenSampleSource
    (new Stream::AudiereFile(input));
  if(!sample)
    fail("Couldn't load stream");

  buf = CreateSampleBuffer(sample);  
}

InputStream *AudiereSource::getStream()
{
  return new AudiereStream(buf->openStream());
}

// --- InputStream ---

AudiereStream::AudiereStream(SampleSourcePtr _sample)
  : sample(_sample), pullSize(0)
{
  assert(sample);

  SampleFormat fmt;
  int channels, rate;
  sample->getFormat(channels, rate, fmt);

  // Calculate the size of one frame
  frameSize = GetSampleSize(fmt) * channels;

  // Make sure that our pullover hack will work. Increase this size if
  // this doesn't work in all cases.
  assert(frameSize <= PSIZE);
}

void AudiereStream::getInfo(int32_t *rate, int32_t *channels, int32_t *bits)
{
  SampleFormat fmt;
  sample->getFormat(*channels, *rate, fmt);
  if(fmt == SF_U8)
    *bits = 8;
  else if(fmt == SF_S16)
    *bits = 16;
  else assert(0);
}

/*
  Get data. Since Audiere operates with frames, not bytes, there's a
  little conversion magic going on here. We need to make sure we're
  reading a whole number of frames - if not, we need to store the
  remainding part of the last frame and remember it for the next read
  operation.

 */
uint32_t AudiereStream::getData(void *_data, uint32_t length)
{
  char *data = (char*)_data;

  // Move the remains from the last operation first
  if(pullSize)
    {
      // pullSize is how much was stored the last time, so skip that.
      memcpy(data, pullOver+pullSize, PSIZE-pullSize);
      length -= pullSize;
      data += pullSize;
    }

  // Determine the overshoot up front
  pullSize = length % frameSize;

  // Number of whole frames
  int frames = length / frameSize;

  // Read the data
  int res = sample->read(frames, data);

  // Are we missing data? If res<length and we're at the end of the
  // stream, then this doesn't apply.
  if(res == frames && pullSize &&
     // Read one more sample
     (sample->read(1, pullOver) != 0))
    {
      // Now, move as much of it as we can fit into the output
      // data
      memcpy(data+length-pullSize, pullOver, pullSize);
    }
  else pullSize = 0;

  // Return the total number of bytes stored
  return frameSize*res + pullSize;
}
