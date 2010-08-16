#include "libsndfile.hpp"

#include "../../tools/str_exception.hpp"
#include <sndfile.h>

using namespace Mangle::Stream;

static void fail(const std::string &msg)
{ throw str_exception("Mangle::libsndfile: " + msg); }

using namespace Mangle::Sound;

void SndFileSource::getInfo(int32_t *_rate, int32_t *_channels, int32_t *_bits)
{
  *_rate = rate;
  *_channels = channels;
  *_bits = bits;
}

size_t SndFileSource::readSamples(void *data, size_t length)
{
  // Read frames. We count channels as part of the frame. Even though
  // libsndfile does not, since it still requires the number of frames
  // read to be a multiple of channels.
  return channels*sf_read_short((SNDFILE*)handle, (short*)data, length*channels);
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
