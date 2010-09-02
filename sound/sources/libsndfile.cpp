#include "libsndfile.hpp"

#include <stdexcept>
#include <sndfile.h>

using namespace Mangle::Stream;

static void fail(const std::string &msg)
{ throw std::runtime_error("Mangle::libsndfile: " + msg); }

using namespace Mangle::Sound;

void SndFileSource::getInfo(int32_t *_rate, int32_t *_channels, int32_t *_bits)
{
  *_rate = rate;
  *_channels = channels;
  *_bits = bits;
}

size_t SndFileSource::readSamples(void *data, size_t length)
{
  // readf_* reads entire frames, including channels
  return sf_readf_short((SNDFILE*)handle, (short*)data, length);
}

SndFileSource::SndFileSource(const std::string &file)
{
  SF_INFO info;
  info.format = 0;
  handle = sf_open(file.c_str(), SFM_READ, &info);
  if(handle == NULL)
    fail("Failed to open " + file);

  // I THINK that using sf_read_short forces the library to convert to
  // 16 bits no matter what, but the libsndfile docs aren't exactly
  // very clear on this point.
  channels = info.channels;
  rate = info.samplerate;
  bits = 16;

  // 16 bits per sample times number of channels
  setup(2*channels);
}

SndFileSource::~SndFileSource()
{
  sf_close((SNDFILE*)handle);
}
