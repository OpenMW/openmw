#ifndef MANGLE_SOUND_AUDIERE_INPUT_H
#define MANGLE_SOUND_AUDIERE_INPUT_H

#include "../input.h"

#include <audiere.h>

namespace Mangle {
namespace Sound {

/// Implementation of Sound::InputManager for Audiere
class AudiereInput : public InputManager
{
 public:
  AudiereInput();

  /// Load a source from a file
  InputSource *load(const std::string &file);

  /// Load a source from a stream
  virtual InputSource *load(Stream::InputStream *input);
};

/// Audiere InputSource implementation
class AudiereSource : public InputSource
{
  audiere::SampleBufferPtr buf;

 public:
  AudiereSource(const std::string &file);
  InputStream *getStream();
  void drop() { delete this; }
};

/// Audiere InputStream implementation
class AudiereStream : public InputStream
{
  audiere::SampleSourcePtr sample;
  int frameSize; // Size of one frame, in bytes

  static const int PSIZE = 10;

  // Temporary storage for unevenly read samples. See the comment for
  // getData() in the .cpp file.
  char pullOver[PSIZE];
  // How much of the above buffer is in use
  int pullSize;

 public:
  AudiereStream(audiere::SampleSourcePtr _sample);

  void getInfo(int32_t *rate, int32_t *channels, int32_t *bits);
  uint32_t getData(void *data, uint32_t length);
  void drop() { delete this; }
};

}} // Namespace
#endif
