#ifndef MANGLE_SOUND_OPENAL_OUT_H
#define MANGLE_SOUND_OPENAL_OUT_H

#include "../output.hpp"

#include <AL/al.h>
#include <AL/alc.h>
#include <list>

namespace Mangle {
namespace Sound {

/// OpenAL sound output
class OpenAL_Sound : public Sound
{
 protected:
  ALuint inst;
  ALuint bufferID;

  // Poor mans reference counting. Might improve this later.
  int *refCnt;

 public:
  /// Read samples from the given input buffer
  OpenAL_Sound(SampleSourcePtr input);

  /// Play an existing buffer, with a given ref counter. Used
  /// internally for cloning.
  OpenAL_Sound(ALuint buf, int *ref);

  ~OpenAL_Sound();

  void play();
  void stop();
  void pause();
  bool isPlaying() const;
  void setVolume(float);
  void setPos(float x, float y, float z);
  void setRepeat(bool);
  void setStreaming(bool) {} // Not implemented yet
  SoundPtr clone() const;

  /// Not implemented
  void setPan(float) {}
};

class OpenAL_Factory : public SoundFactory
{
  ALCdevice  *Device;
  ALCcontext *Context;
  bool didSetup;

 public:
  /// Initialize object. Pass true (default) if you want the
  /// constructor to set up the current ALCdevice and ALCcontext for
  /// you.
  OpenAL_Factory(bool doSetup = true);
  ~OpenAL_Factory();

  SoundPtr load(const std::string &file) { assert(0); }
  SoundPtr load(Stream::StreamPtr input) { assert(0); }
  SoundPtr loadRaw(SampleSourcePtr input)
  { return SoundPtr(new OpenAL_Sound(input)); }

  void update() {}
  void setListenerPos(float x, float y, float z,
                      float fx, float fy, float fz,
                      float ux, float uy, float uz)
    {
      ALfloat orient[6];
      orient[0] = fx;
      orient[1] = fy;
      orient[2] = fz;
      orient[3] = ux;
      orient[4] = uy;
      orient[5] = uz;
      alListener3f(AL_POSITION, x, y, z);
      alListenerfv(AL_ORIENTATION, orient);
    }
};

}} // namespaces
#endif
