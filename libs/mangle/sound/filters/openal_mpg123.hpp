#ifndef MANGLE_MPG123_OPENAL_H
#define MANGLE_MPG123_OPENAL_H

#include "input_filter.hpp"
#include "../sources/mpg123_source.hpp"
#include "../outputs/openal_out.hpp"

namespace Mangle {
namespace Sound {

/// A InputFilter that adds mpg123 decoding to OpenAL. Only supports
/// MP3 files.
class OpenAL_Mpg123_Factory : public InputFilter
{
 public:
  OpenAL_Mpg123_Factory()
    {
      set(SoundFactoryPtr(new OpenAL_Factory),
          SampleSourceLoaderPtr(new Mpg123Loader));
    }
};

}}
#endif
