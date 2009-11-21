#ifndef MANGLE_FFMPEG_OPENAL_H
#define MANGLE_FFMPEG_OPENAL_H

#include "input_filter.h"
#include "input_ffmpeg.h"
#include "output_openal.h"

namespace Mangle {
namespace Sound {

/// A PairManager filter that adds FFmpeg decoding to OpenAL
class OpenAL_FFM_Manager : public InputFilter
{
 public:
  OpenAL_FFM_Manager()
    {
      set(new OpenAL_Manager,
          new FFM_InputManager);
    }
  ~OpenAL_FFM_Manager()
    {
      delete snd;
      delete inp;
    }
};

}}
#endif
