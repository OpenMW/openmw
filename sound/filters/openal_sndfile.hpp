#ifndef MANGLE_SNDFILE_OPENAL_H
#define MANGLE_SNDFILE_OPENAL_H

#include "input_filter.hpp"
#include "../sources/libsndfile.hpp"
#include "../outputs/openal_out.hpp"

namespace Mangle {
namespace Sound {

/// A InputFilter that adds libsnd decoding to OpenAL. libsndfile
/// supports most formats except MP3.
class OpenAL_SndFile_Factory : public InputFilter
{
 public:
  OpenAL_SndFile_Factory()
    {
      set(SoundFactoryPtr(new OpenAL_Factory),
          SampleSourceLoaderPtr(new SndFileLoader));
    }
};

}}
#endif
