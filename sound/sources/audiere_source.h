#ifndef MANGLE_SOUND_AUDIERE_SOURCE_H
#define MANGLE_SOUND_AUDIERE_SOURCE_H

#include "../source.h"

#include <audiere.h>

namespace Mangle {
namespace Sound {

/// A sample source that decodes files using Audiere
class AudiereSource : public SampleSource
{
  audiere::SampleSourcePtr sample;

  // Number of bytes we cache between reads. This should correspond to
  // the maximum possible value of frameSize.
  static const int PSIZE = 10;

  // Size of one frame, in bytes
  int frameSize;

  // Temporary storage for unevenly read samples. See the comment for
  // read() in the .cpp file.
  char pullOver[PSIZE];
  // How much of the above buffer is in use
  int pullSize;

  void getFormat();

 public:
  /// Decode the given sound file
  AudiereSource(const std::string &file);

  /// Decode the given sound stream
  AudiereSource(Stream::Stream *src);

  /// Read directly from an existing audiere::SampleSource
  AudiereSource(audiere::SampleSourcePtr src);

  void getInfo(int32_t *rate, int32_t *channels, int32_t *bits);
  size_t read(void *data, size_t length);
};

#include "loadertemplate.h"

/// A factory that loads AudiereSources from file and stream
typedef SSL_Template<AudiereSource,true,true> AudiereLoader;

}} // Namespace
#endif
