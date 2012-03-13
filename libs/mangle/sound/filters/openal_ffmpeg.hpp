#ifndef MANGLE_FFMPEG_OPENAL_H
#define MANGLE_FFMPEG_OPENAL_H

#include "input_filter.hpp"
#include "../sources/ffmpeg_source.hpp"
#include "../outputs/openal_out.hpp"

namespace Mangle {
namespace Sound {

/// A InputFilter that adds ffmpeg decoding to OpenAL.
class OpenAL_FFMpeg_Factory : public InputFilter
{
 public:
  OpenAL_FFMpeg_Factory()
    {
      set(SoundFactoryPtr(new OpenAL_Factory),
          SampleSourceLoaderPtr(new FFMpegLoader));
    }
};

}}
#endif
