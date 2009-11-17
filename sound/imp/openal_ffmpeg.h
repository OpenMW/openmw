#ifndef GOOI_FFMPEG_OPENAL_H
#define GOOI_FFMPEG_OPENAL_H

#include "sound_pair.h"
#include "input_ffmpeg.h"
#include "output_openal.h"

namespace GOOI {
namespace Sound {

/// A PairManager filter that adds FFmpeg decoding to OpenAL
class OpenAL_FFM_Manager : public PairManager
{
 public:
  OpenAL_FFM_Manager()
    {
      set(new FFM_InputManager,
          new OpenAL_Manager);
    }
  ~OpenAL_FFM_Manager()
    {
      delete snd;
      delete inp;
    }
};

}}
#endif
