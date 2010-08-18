#ifndef MANGLE_VARIOUS_OPENAL_H
#define MANGLE_VARIOUS_OPENAL_H

#include "input_filter.hpp"
#include "source_splicer.hpp"
#include "../sources/mpg123_source.hpp"
#include "../sources/wav_source.hpp"
#include "../outputs/openal_out.hpp"

namespace Mangle {
namespace Sound {

/** A InputFilter that uses OpenAL for output, and load input from
    various individual sources, depending on file extension. Currently
    supports:

      MP3: mpg123
      WAV: custom wav loader (PCM only)

    This could be an alternative to using eg. libsndfile or other 3rd
    party decoder libraries. (We implemented this for OpenMW because
    we were experiencing crashes when using libsndfile.)
 */
class OpenAL_Various_Factory : public InputFilter
{
 public:
  OpenAL_Various_Factory()
    {
      SourceSplicer *splice = new SourceSplicer;

      splice->add("mp3", SampleSourceLoaderPtr(new Mpg123Loader));
      splice->add("wav", SampleSourceLoaderPtr(new WavLoader));

      set(SoundFactoryPtr(new OpenAL_Factory),
          SampleSourceLoaderPtr(splice));
    }
};

}}
#endif
