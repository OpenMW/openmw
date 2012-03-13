#ifndef MANGLE_SNDFILE_MPG123_OPENAL_H
#define MANGLE_SNDFILE_MPG123_OPENAL_H

#include "input_filter.hpp"
#include "source_splicer.hpp"
#include "../sources/mpg123_source.hpp"
#include "../sources/libsndfile.hpp"
#include "../outputs/openal_out.hpp"

namespace Mangle {
namespace Sound {

/// A InputFilter that uses OpenAL for output, and mpg123 (for MP3) +
/// libsndfile (for everything else) to decode files. Can only load
/// from the file system, and uses the file name to differentiate
/// between mp3 and non-mp3 types.
class OpenAL_SndFile_Mpg123_Factory : public InputFilter
{
 public:
  OpenAL_SndFile_Mpg123_Factory()
    {
      SourceSplicer *splice = new SourceSplicer;

      splice->add("mp3", SampleSourceLoaderPtr(new Mpg123Loader));
      splice->setDefault(SampleSourceLoaderPtr(new SndFileLoader));

      set(SoundFactoryPtr(new OpenAL_Factory),
          SampleSourceLoaderPtr(splice));
    }
};

}}
#endif
