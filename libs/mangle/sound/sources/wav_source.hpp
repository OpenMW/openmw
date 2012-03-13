#ifndef MANGLE_SOUND_WAV_SOURCE_H
#define MANGLE_SOUND_WAV_SOURCE_H

#include "../source.hpp"
#include <assert.h>

namespace Mangle {
namespace Sound {

/// WAV file decoder. Has no external library dependencies.
class WavSource : public SampleSource
{
  // Sound info
  uint32_t rate, channels, bits;

  // Total size (of output) and bytes left
  uint32_t total, left;

  // Offset in input of the beginning of the data block
  size_t dataOffset;

  Mangle::Stream::StreamPtr input;

  void open(Mangle::Stream::StreamPtr);

 public:
  /// Decode the given sound file
  WavSource(const std::string&);

  /// Decode from stream
  WavSource(Mangle::Stream::StreamPtr s)
  { open(s); }

  void getInfo(int32_t *rate, int32_t *channels, int32_t *bits);
  size_t read(void *data, size_t length);

  void seek(size_t);
  size_t tell() const { return total-left; }
  size_t size() const { return total; }
  bool eof() const { return left > 0; }
};

#include "loadertemplate.hpp"

/// A factory that loads WavSources from file and stream
typedef SSL_Template<WavSource,true,true> WavLoader;

}} // Namespace
#endif
