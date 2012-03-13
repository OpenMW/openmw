#ifndef MANGLE_SOUND_MPG123_SOURCE_H
#define MANGLE_SOUND_MPG123_SOURCE_H

#include "../source.hpp"
#include <assert.h>

namespace Mangle {
namespace Sound {

/// A sample source that decodes files using libmpg123. Only supports
/// MP3 files.
class Mpg123Source : public SampleSource
{
  void *mh;
  long int rate;
  int channels, bits;

 public:
  /// Decode the given sound file
  Mpg123Source(const std::string &file);

  /// Needed by SSL_Template but not yet supported
  Mpg123Source(Mangle::Stream::StreamPtr data)
  { assert(0); }

  ~Mpg123Source();

  void getInfo(int32_t *rate, int32_t *channels, int32_t *bits);
  size_t read(void *data, size_t length);
};

#include "loadertemplate.hpp"

/// A factory that loads Mpg123Sources from file and stream
struct Mpg123Loader : SSL_Template<Mpg123Source,false,true>
{
  /** Sets up libmpg123 for you, and closes it on destruction. If you
      want to do this yourself, send setup=false.
   */
  Mpg123Loader(bool setup=true);
  ~Mpg123Loader();
private:
  bool didSetup;
};

}} // Namespace
#endif
