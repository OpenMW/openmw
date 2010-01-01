#ifndef MANGLE_SOUND_STREAMSOURCE_H
#define MANGLE_SOUND_STREAMSOURCE_H

#include "../source.h"

namespace Mangle {
namespace Sound {

/// A class for reading raw samples directly from a stream.
class Stream2Samples : public SampleSource
{
  int32_t rate, channels, bits;
  Mangle::Stream::StreamPtr inp;

 public:
  Stream2Samples(Mangle::Stream::StreamPtr _inp, int32_t _rate, int32_t _channels, int32_t _bits)
   : inp(_inp), rate(_rate), channels(_channels), bits(_bits)
    {
      isSeekable = inp->isSeekable;
      hasPosition = inp->hasPosition;
      hasSize = inp->hasSize;
      hasPtr = inp->hasPtr;
    }

  /// Get the sample rate, number of channels, and bits per
  /// sample. NULL parameters are ignored.
  void getInfo(int32_t *_rate, int32_t *_channels, int32_t *_bits)
  {
    if(_rate) *_rate = rate;
    if(_channels) *_channels = channels;
    if(_bits) *_bits = bits;
  }

  size_t read(void *out, size_t count)
    { return inp->read(out, count); }

  void seek(size_t pos) { inp->seek(pos); }
  size_t tell() const { return inp->tell(); }
  size_t size() const { return inp->size(); }
  bool eof() const { return inp->eof(); }
  const void *getPtr() { return inp->getPtr(); }
  const void *getPtr(size_t size) { return inp->getPtr(size); }
  const void *getPtr(size_t pos, size_t size) { return inp->getPtr(pos, size); }
};

}} // namespaces
#endif
