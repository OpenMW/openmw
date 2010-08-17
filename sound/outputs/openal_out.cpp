#include "openal_out.hpp"
#include <assert.h>

#include "../../stream/filters/buffer_stream.hpp"
#include "../../tools/str_exception.hpp"

#include <AL/al.h>
#include <AL/alc.h>

using namespace Mangle::Sound;

// ---- Helper functions and classes ----

static void fail(const std::string &msg)
{ throw str_exception("OpenAL exception: " + msg); }

/*
  Check for AL error. Since we're always calling this with string
  literals, and it only makes sense to optimize for the non-error
  case, the parameter is const char* rather than std::string.

  This way we don't force the compiler to create a string object each
  time we're called (since the string is never used unless there's an
  error), although a good compiler might have optimized that away in
  any case.
 */
static void checkALError(const char *where)
{
  ALenum err = alGetError();
  if(err != AL_NO_ERROR)
    {
      std::string msg = where;

      const ALchar* errmsg = alGetString(err);
      if(errmsg)
        fail("\"" + std::string(alGetString(err)) + "\" while " + msg);
      else
        fail("non-specified error while " + msg + " (did you forget to initialize OpenAL?)");
    }
}

static void getALFormat(SampleSourcePtr inp, int &fmt, int &rate)
{
  int ch, bits;
  inp->getInfo(&rate, &ch, &bits);

  fmt = 0;

  if(bits == 8)
    {
      if(ch == 1) fmt = AL_FORMAT_MONO8;
      if(ch == 2) fmt = AL_FORMAT_STEREO8;
      if(alIsExtensionPresent("AL_EXT_MCFORMATS"))
        {
          if(ch == 4) fmt = alGetEnumValue("AL_FORMAT_QUAD8");
          if(ch == 6) fmt = alGetEnumValue("AL_FORMAT_51CHN8");
        }
    }
  if(bits == 16)
    {
      if(ch == 1) fmt = AL_FORMAT_MONO16;
      if(ch == 2) fmt = AL_FORMAT_STEREO16;
      if(ch == 4) fmt = alGetEnumValue("AL_FORMAT_QUAD16");
      if(alIsExtensionPresent("AL_EXT_MCFORMATS"))
        {
          if(ch == 4) fmt = alGetEnumValue("AL_FORMAT_QUAD16");
          if(ch == 6) fmt = alGetEnumValue("AL_FORMAT_51CHN16");
        }
    }

  if(fmt == 0)
    fail("Unsupported input format");
}

/// OpenAL sound output
class OpenAL_Sound : public Sound
{
  ALuint inst;
  ALuint bufferID;

  // Poor mans reference counting. Might improve this later. When
  // NULL, the buffer has not been set up yet.
  int *refCnt;

  bool streaming;

  // Input stream
  SampleSourcePtr input;

  void setupBuffer();

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
  void setPitch(float);
  void setRepeat(bool);
  void setStreaming(bool s) { streaming = s; }
  SoundPtr clone();

  // a = AL_REFERENCE_DISTANCE
  // b = AL_MAX_DISTANCE
  // c = ignored
  void setRange(float a, float b=0.0, float c=0.0);

  /// Not implemented
  void setPan(float) {}
};

// ---- OpenAL_Factory ----

SoundPtr OpenAL_Factory::loadRaw(SampleSourcePtr input)
{
  return SoundPtr(new OpenAL_Sound(input));
}

void OpenAL_Factory::update()
{
}

void OpenAL_Factory::setListenerPos(float x, float y, float z,
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

OpenAL_Factory::OpenAL_Factory(bool doSetup)
  : device(NULL), context(NULL), didSetup(doSetup)
{
  needsUpdate = true;
  has3D = true;
  canLoadFile = false;
  canLoadStream = false;
  canLoadSource = true;

  ALCdevice *Device;
  ALCcontext *Context;

  if(doSetup)
    {
      // Set up sound system
      Device = alcOpenDevice(NULL);
      Context = alcCreateContext(Device, NULL);

      if(!Device || !Context)
        fail("Failed to initialize context or device");

      alcMakeContextCurrent(Context);

      device = Device;
      context = Context;
    }
}

OpenAL_Factory::~OpenAL_Factory()
{
  // Deinitialize sound system
  if(didSetup)
    {
      alcMakeContextCurrent(NULL);
      if(context) alcDestroyContext((ALCcontext*)context);
      if(device) alcCloseDevice((ALCdevice*)device);
    }
}

// ---- OpenAL_Sound ----

void OpenAL_Sound::play()
{
  setupBuffer();
  alSourcePlay(inst);
  checkALError("starting playback");
}

void OpenAL_Sound::stop()
{
  alSourceStop(inst);
  checkALError("stopping");
}

void OpenAL_Sound::pause()
{
  alSourcePause(inst);
  checkALError("pausing");
}

bool OpenAL_Sound::isPlaying() const
{
  ALint state;
  alGetSourcei(inst, AL_SOURCE_STATE, &state);

  return state == AL_PLAYING;
}

void OpenAL_Sound::setVolume(float volume)
{
  if(volume > 1.0) volume = 1.0;
  if(volume < 0.0) volume = 0.0;
  alSourcef(inst, AL_GAIN, volume);
  checkALError("setting volume");
}

void OpenAL_Sound::setRange(float a, float b, float)
{
  alSourcef(inst, AL_REFERENCE_DISTANCE, a);
  alSourcef(inst, AL_MAX_DISTANCE, b);
  checkALError("setting sound ranges");
}

void OpenAL_Sound::setPos(float x, float y, float z)
{
  alSource3f(inst, AL_POSITION, x, y, z);
  checkALError("setting position");
}

void OpenAL_Sound::setPitch(float pitch)
{
  alSourcef(inst, AL_PITCH, pitch);
  checkALError("setting pitch");
}

void OpenAL_Sound::setRepeat(bool rep)
{
  alSourcei(inst, AL_LOOPING, rep?AL_TRUE:AL_FALSE);
}

SoundPtr OpenAL_Sound::clone()
{
  setupBuffer();
  assert(!streaming && "cloning streamed sounds not supported");
  return SoundPtr(new OpenAL_Sound(bufferID, refCnt));
}

// Constructor used for cloned sounds
OpenAL_Sound::OpenAL_Sound(ALuint buf, int *ref)
  : bufferID(buf), refCnt(ref), streaming(false)
{
  // Increase the reference count
  assert(ref != NULL);
  *refCnt++;

  // Create a source
  alGenSources(1, &inst);
  checkALError("creating instance (clone)");
  alSourcei(inst, AL_BUFFER, bufferID);
  checkALError("assigning buffer (clone)");
}

// Constructor used for original (non-cloned) sounds
OpenAL_Sound::OpenAL_Sound(SampleSourcePtr _input)
  : bufferID(0), refCnt(NULL), streaming(false), input(_input)
{
  // Create a source
  alGenSources(1, &inst);
  checkALError("creating source");

  // By default, the sound starts out in a buffer-less mode. We don't
  // create a buffer until the sound is played. This gives the user
  // the chance to call setStreaming(true) first.
}

void OpenAL_Sound::setupBuffer()
{
  if(refCnt != NULL) return;

  assert(input);

  // Get the format
  int fmt, rate;
  getALFormat(input, fmt, rate);

  if(streaming)
    {
      // To be done
    }

  // NON-STREAMING

  // Set up the OpenAL buffer
  alGenBuffers(1, &bufferID);
  checkALError("generating buffer");
  assert(bufferID != 0);

  // Does the stream support pointer operations?
  if(input->hasPtr)
    {
      // If so, we can read the data directly from the stream
      alBufferData(bufferID, fmt, input->getPtr(), input->size(), rate);
    }
  else
    {
      // Read the entire stream into a temporary buffer first
      Mangle::Stream::BufferStream buf(input, streaming?1024*1024:32*1024);

      // Then copy that into OpenAL
      alBufferData(bufferID, fmt, buf.getPtr(), buf.size(), rate);
    }
  checkALError("loading sound buffer");

  // We're done with the input stream, release the pointer
  input.reset();

  alSourcei(inst, AL_BUFFER, bufferID);
  checkALError("assigning buffer");

  // Create a cheap reference counter for the buffer
  refCnt = new int;
  *refCnt = 1;
}

OpenAL_Sound::~OpenAL_Sound()
{
  // Stop
  alSourceStop(inst);

  // Return sound
  alDeleteSources(1, &inst);

  // Decrease the reference counter
  if((-- *refCnt) == 0)
    {
      // We're the last owner. Delete the buffer and the counter
      // itself.
      alDeleteBuffers(1, &bufferID);
      checkALError("deleting buffer");
      delete refCnt;
    }
}
