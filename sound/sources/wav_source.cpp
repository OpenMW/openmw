#include "wav_source.hpp"

#include "../../tools/str_exception.hpp"
#include "../../stream/servers/file_stream.hpp"

using namespace Mangle::Stream;
using namespace Mangle::Sound;

static void fail(const std::string &msg)
{ throw str_exception("Mangle::Wav exception: " + msg); }

void WavSource::getInfo(int32_t *pRate, int32_t *pChannels, int32_t *pBits)
{
  // Use the values we found in the constructor
  *pRate = rate;
  *pChannels = channels;
  *pBits = bits;
}

void WavSource::seek(size_t pos)
{
  // Seek the stream and set 'left'
  assert(isSeekable);
  if(pos > total) pos = total;
  input->seek(dataOffset + pos);
  left = total-pos;
}

size_t WavSource::read(void *data, size_t length)
{
  if(length > left)
    length = left;
  input->read(data, length);
  return length;
}

void WavSource::open(Mangle::Stream::StreamPtr data)
{
  input = data;

  hasPosition = true;
  hasSize = true;
  // If we can check position and seek in the input stream, then we
  // can seek the wav data too.
  isSeekable = input->isSeekable && input->hasPosition;

  // Read header
  unsigned int val;

  input->read(&val,4);  // header
  if(val != 0x46464952) // "RIFF"
    fail("Not a WAV file");

  input->read(&val,4);  // size (ignored)
  input->read(&val,4);  // file format
  if(val != 0x45564157) // "WAVE"
    fail("Not a valid WAV file");

  input->read(&val,4);  // "fmt "
  input->read(&val,4);  // chunk size (must be 16)
  if(val != 16)
    fail("Unsupported WAV format");

  input->read(&val,2);
  if(val != 1)
    fail("Non-PCM (compressed) WAV files not supported");

  // Sound data specification
  channels = 0;
  input->read(&channels,2);
  input->read(&rate, 4);

  // Skip next 6 bytes
  input->read(&val, 4);
  input->read(&val, 2);

  // Bits per sample
  bits = 0;
  input->read(&bits,2);

  input->read(&val,4);  // Data header
  if(val != 0x61746164) // "data"
    fail("Expected data block");

  // Finally, read the data size
  input->read(&total,4);
  left = total;

  // Store the beginning of the data block for later
  if(input->hasPosition)
    dataOffset = input->tell();
}

WavSource::WavSource(const std::string &file)
{ open(StreamPtr(new FileStream(file))); }
