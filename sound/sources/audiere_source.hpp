#ifndef MANGLE_SOUND_AUDIERE_SOURCE_H
#define MANGLE_SOUND_AUDIERE_SOURCE_H

#include "sample_reader.hpp"

// audiere.h from 1.9.4 (latest) release uses
// cstring routines like strchr() and strlen() without
// including cstring itself.
#include <cstring>
#include <audiere.h>

namespace Mangle {
namespace Sound {

/// A sample source that decodes files using Audiere
class AudiereSource : public SampleReader
{
  audiere::SampleSourcePtr sample;

  size_t readSamples(void *data, size_t length)
  { return sample->read(length, data); }

  void doSetup();

 public:
  /// Decode the given sound file
  AudiereSource(const std::string &file);

  /// Decode the given sound stream
  AudiereSource(Mangle::Stream::StreamPtr src);

  /// Read directly from an existing audiere::SampleSource
  AudiereSource(audiere::SampleSourcePtr src);

  void getInfo(int32_t *rate, int32_t *channels, int32_t *bits);

  void seek(size_t pos) { sample->setPosition(pos/frameSize); }
  size_t tell() const { return sample->getPosition()*frameSize; }
  size_t size() const { return sample->getLength()*frameSize; }
};

#include "loadertemplate.hpp"

/// A factory that loads AudiereSources from file and stream
typedef SSL_Template<AudiereSource,true,true> AudiereLoader;

}} // Namespace
#endif
