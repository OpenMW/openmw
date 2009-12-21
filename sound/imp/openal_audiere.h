#ifndef MANGLE_FFMPEG_OPENAL_H
#define MANGLE_FFMPEG_OPENAL_H

#include "input_filter.h"
#include "input_audiere.h"
#include "output_openal.h"

namespace Mangle {
namespace Sound {

/// A InputFilter that adds audiere decoding to OpenAL. Audiere has
/// it's own output, but OpenAL sports 3D and other advanced features.
class OpenAL_Audiere_Manager : public InputFilter
{
 public:
  OpenAL_Audiere_Manager()
    {
      set(new OpenAL_Manager,
          new AudiereInput);
    }
  ~OpenAL_Audiere_Manager()
    {
      delete snd;
      delete inp;
    }
};

}}
#endif
