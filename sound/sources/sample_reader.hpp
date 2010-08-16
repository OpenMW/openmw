#ifndef MANGLE_SOUND_SAMPLE_READER_H
#define MANGLE_SOUND_SAMPLE_READER_H

#include "../source.hpp"

namespace Mangle {
namespace Sound {

  /* This is a helper base class for other SampleSource
     implementations. Certain sources (like Audiere and libsndfile)
     insist on reading whole samples rather than bytes. This class
     compensates for that, and allows you to read bytes rather than
     samples.
   */
class SampleReader : public SampleSource
{
  // Pullover buffer
  char* pullOver;

  // How much of the above buffer is in use.
  int pullSize;

protected:
  // Size of one frame, in bytes. This is also the size of the
  // pullOver buffer.
  int frameSize;

  // MUST be called by base class constructor. The parameter gives the
  // size of one sample/frame, in bytes.
  void setup(int);

  // Read the given number of samples, in multiples of frameSize. Does
  // not have to set or respect isEof.
  virtual size_t readSamples(void *data, size_t num) = 0;

 public:
  SampleReader() : pullOver(NULL) {}
  ~SampleReader();
  size_t read(void *data, size_t length);
};
}} // Namespace
#endif
