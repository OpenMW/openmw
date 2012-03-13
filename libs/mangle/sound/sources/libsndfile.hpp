#ifndef MANGLE_SOUND_SNDFILE_SOURCE_H
#define MANGLE_SOUND_SNDFILE_SOURCE_H

#include "sample_reader.hpp"

namespace Mangle {
namespace Sound {

/// A sample source that decodes files using libsndfile. Supports most
/// formats except mp3.
class SndFileSource : public SampleReader
{
  void *handle;
  int channels, rate, bits;

  size_t readSamples(void *data, size_t length);

 public:
  /// Decode the given sound file
  SndFileSource(const std::string &file);

  /// Decode the given sound stream (not supported)
  SndFileSource(Mangle::Stream::StreamPtr src) { assert(0); }

  ~SndFileSource();

  void getInfo(int32_t *rate, int32_t *channels, int32_t *bits);
};

#include "loadertemplate.hpp"

/// A factory that loads SndFileSources from file and stream
typedef SSL_Template<SndFileSource,false,true> SndFileLoader;

}} // Namespace
#endif
