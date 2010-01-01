#ifndef MANGLE_SOUND_OPENAL_OUT_H
#define MANGLE_SOUND_OPENAL_OUT_H

#include "../output.h"
#include "../../stream/filters/buffer_stream.h"

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
  OpenAL_Sound(SampleSourcePtr input);
  OpenAL_Sound(ALuint buf, int *ref); // Used for cloning
  ~OpenAL_Sound();

  void play();
  void stop();
  void pause();
  bool isPlaying() const;
  void setVolume(float);
  void setPos(float x, float y, float z);
  void setRepeat(bool);
  void setStreaming(bool) {} // Not implemented yet
  Sound* clone() const;

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
  OpenAL_Factory(bool doSetup = true)
   : didSetup(doSetup)
    {
      needsUpdate = false;
      has3D = true;
      canLoadFile = false;
      canLoadStream = false;
      canLoadSource = true;

      if(doSetup)
        {
          // Set up sound system
          Device = alcOpenDevice(NULL);
          Context = alcCreateContext(Device, NULL);

          if(!Device || !Context)
            fail("Failed to initialize context or device");

          alcMakeContextCurrent(Context);
        }
    }

  ~OpenAL_Factory()
    {
      // Deinitialize sound system
      if(didSetup)
        {
          alcMakeContextCurrent(NULL);
          if(Context) alcDestroyContext(Context);
          if(Device) alcCloseDevice(Device);
        }
    }

  SoundPtr load(const std::string &file, bool stream=false) { assert(0); }
  SoundPtr load(Stream::StreamPtr input, bool stream=false) { assert(0); }
  SoundPtr load(SampleSourcePtr input, bool stream=false)
  { return SoundPtr(new OpenAL_Sound(input)); }

  void update() {}
  setListenerPos(float x, float y, float z,
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
