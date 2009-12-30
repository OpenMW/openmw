#include "audiere_source.h"

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

// --- SampleSource ---

void AudiereSource::getInfo(int32_t *rate, int32_t *channels, int32_t *bits)
{
  SampleFormat fmt;
  sample->getFormat(*channels, *rate, fmt);
  if(bits)
    {
      if(fmt == SF_U8)
        *bits = 8;
      else if(fmt == SF_S16)
        *bits = 16;
      else assert(0);
    }
}

/*
  Get data. Since Audiere operates with frames, not bytes, there's a
  little conversion magic going on here. We need to make sure we're
  reading a whole number of frames - if not, we need to store the
  remainding part of the last frame and remember it for the next read
  operation.
 */
size_t AudiereSource::read(void *_data, size_t length)
{
  if(isEof) return 0;

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

  if(res < frames)
    isEof = true;

  // Are we missing data? If we're at the end of the stream, then this
  // doesn't apply.
  if(!isEof && pullSize)
    {
      // Read one more sample
      if(sample->read(1, pullOver) != 0)
        {
          // Then, move as much of it as we can fit into the output
          // data
          memcpy(data+length-pullSize, pullOver, pullSize);
        }
      else
        // Failed reading, we're out of data
        isEof = true;
    }

  // If we're at the end of the stream, then no data remains to be
  // pulled over
  if(isEof)
    pullSize = 0;

  // Return the total number of bytes stored
  return frameSize*res + pullSize;
}

// --- Constructors ---

AudiereSource::AudiereSource(const std::string &file)
{
  sample = OpenSampleSource(file.c_str());

  if(!sample)
    fail("Couldn't load file " + file);

  setup();
}

AudiereSource::AudiereSource(Stream::Stream *input)
{
  // Use our Stream::AudiereFile implementation to convert a Mangle
  // 'Stream' to an Audiere 'File'
  sample = OpenSampleSource(new Stream::AudiereFile(input));
  if(!sample)
    fail("Couldn't load stream");

  setup();
}

AudiereSource::AudiereSource(audiere::SampleSourcePtr src)
  : sample(src)
{ assert(sample); setup(); }

// Common function called from all constructors
AudiereSource::setup()
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

  isSeekable = sample->isSeekable();
  hasPosition = true;
  hasSize = true;
}
