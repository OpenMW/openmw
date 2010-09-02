#include "audiere_source.hpp"

#include "../../stream/clients/audiere_file.hpp"

#include <stdexcept>

using namespace Mangle::Stream;

static void fail(const std::string &msg)
{ throw std::runtime_error("Audiere exception: " + msg); }

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

// --- Constructors ---

AudiereSource::AudiereSource(const std::string &file)
{
  sample = OpenSampleSource(file.c_str());

  if(!sample)
    fail("Couldn't load file " + file);

  doSetup();
}

AudiereSource::AudiereSource(StreamPtr input)
{
  // Use our Stream::AudiereFile implementation to convert a Mangle
  // 'Stream' to an Audiere 'File'
  sample = OpenSampleSource(new AudiereFile(input));
  if(!sample)
    fail("Couldn't load stream");

  doSetup();
}

AudiereSource::AudiereSource(audiere::SampleSourcePtr src)
  : sample(src)
{ assert(sample); doSetup(); }

// Common function called from all constructors
void AudiereSource::doSetup()
{
  assert(sample);

  SampleFormat fmt;
  int channels, rate;
  sample->getFormat(channels, rate, fmt);

  // Calculate the size of one frame, and pass it to SampleReader.
  setup(GetSampleSize(fmt) * channels);

  isSeekable = sample->isSeekable();
  hasPosition = true;
  hasSize = true;
}
