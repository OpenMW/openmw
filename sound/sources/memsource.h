#ifndef MANGLE_SOUND_MEMSOURCE_H
#define MANGLE_SOUND_MEMSOURCE_H

#include "../source.h"

namespace Mangle {
namespace Sound {

/// A sample source reading directly from a memory buffer
class MemorySource : public SampleSource
{
  char *buf;
  size_t len;
  size_t pos;

  int32_t rate, channels, bits;

 public:
 MemorySource(void *_buf, size_t _len, int32_t _rate, int32_t _channels, int32_t _bits)
   : len(_len), pos(0), rate(_rate), channels(_channels), bits(_bits)
    { buf = (char*)_buf; }

  /// Get the sample rate, number of channels, and bits per
  /// sample. NULL parameters are ignored.
  void getInfo(int32_t *_rate, int32_t *_channels, int32_t *_bits) const
  {
    if(_rate) *_rate = rate;
    if(_channels) *_channels = channels;
    if(_bits) *_bits = bits;
  }

  bool eof() const { return pos == len; }

  size_t read(void *out, size_t count)
  {
    assert(len >= pos);

    if(count > (len-pos))
      count = len-pos;

    if(count) memcpy(out, buf+pos, count);
    pos += count;

    return count;
  }
};

}} // namespaces
#endif
