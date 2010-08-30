#ifndef MANGLE_SOUND_OPENAL_OUT_H
#define MANGLE_SOUND_OPENAL_OUT_H

#include "../output.hpp"
#include <list>

namespace Mangle {
namespace Sound {

class OpenAL_Sound;

class OpenAL_Factory : public SoundFactory
{
  void *device;
  void *context;
  bool didSetup;

  // List of streaming sounds that need to be updated every frame.
  typedef std::list<OpenAL_Sound*> StreamList;
  StreamList streaming;

  friend class OpenAL_Sound;
  void notifyStreaming(OpenAL_Sound*);
  void notifyDelete(OpenAL_Sound*);

 public:
  /// Initialize object. Pass true (default) if you want the
  /// constructor to set up the current ALCdevice and ALCcontext for
  /// you.
  OpenAL_Factory(bool doSetup = true);
  ~OpenAL_Factory();

  SoundPtr load(const std::string &file) { assert(0); return SoundPtr(); }
  SoundPtr load(Stream::StreamPtr input) { assert(0); return SoundPtr(); }
  SoundPtr loadRaw(SampleSourcePtr input);

  void update();
  void setListenerPos(float x, float y, float z,
                      float fx, float fy, float fz,
                      float ux, float uy, float uz);
};

}} // namespaces
#endif
