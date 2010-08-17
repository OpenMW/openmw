#ifndef MANGLE_SOUND_OPENAL_OUT_H
#define MANGLE_SOUND_OPENAL_OUT_H

#include "../output.hpp"

namespace Mangle {
namespace Sound {

class OpenAL_Factory : public SoundFactory
{
  void *device;
  void *context;
  bool didSetup;

 public:
  /// Initialize object. Pass true (default) if you want the
  /// constructor to set up the current ALCdevice and ALCcontext for
  /// you.
  OpenAL_Factory(bool doSetup = true);
  ~OpenAL_Factory();

  SoundPtr load(const std::string &file) { assert(0); }
  SoundPtr load(Stream::StreamPtr input) { assert(0); }
  SoundPtr loadRaw(SampleSourcePtr input);

  void update();
  void setListenerPos(float x, float y, float z,
                      float fx, float fy, float fz,
                      float ux, float uy, float uz);
};

}} // namespaces
#endif
