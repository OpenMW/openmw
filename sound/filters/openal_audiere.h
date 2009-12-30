#ifndef MANGLE_FFMPEG_OPENAL_H
#define MANGLE_FFMPEG_OPENAL_H

#include "input_filter.h"
#include "../sources/audiere_source.h"
#include "../outputs/openal_out.h"

namespace Mangle {
namespace Sound {

/// A InputFilter that adds audiere decoding to OpenAL. Audiere has
/// it's own output, but OpenAL sports 3D and other advanced features.
class OpenAL_Audiere_Factory : public InputFilter
{
 public:
  OpenAL_Audiere_Factory()
    {
      set(new OpenAL_Factory,
          new AudiereLoader);
    }
  ~OpenAL_Audiere_Factory()
    {
      delete snd;
      delete inp;
    }
};

}}
#endif
