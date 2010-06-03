#ifndef MANGLE_FFMPEG_OPENAL_H
#define MANGLE_FFMPEG_OPENAL_H

#include "input_filter.hpp"
#include "../sources/audiere_source.hpp"
#include "../outputs/openal_out.hpp"

namespace Mangle {
namespace Sound {

/// A InputFilter that adds audiere decoding to OpenAL. Audiere has
/// it's own output, but OpenAL sports 3D and other advanced features.
class OpenAL_Audiere_Factory : public InputFilter
{
 public:
  OpenAL_Audiere_Factory()
    {
      set(SoundFactoryPtr(new OpenAL_Factory),
          SampleSourceLoaderPtr(new AudiereLoader));
    }
};

}}
#endif
