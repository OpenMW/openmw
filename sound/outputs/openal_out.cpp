#include "openal_out.h"
#include <assert.h>

#include "../../stream/filters/buffer_stream.h"
#include "../../tools/str_exception.h"

using namespace Mangle::Sound;

// ---- Helper functions and classes ----

static void fail(const std::string &msg)
{ throw str_exception("OpenAL exception: " + msg); }

static void checkALError(const std::string &msg)
{
  ALenum err = alGetError();
  if(err != AL_NO_ERROR)
    {
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

// ---- OpenAL_Factory ----

OpenAL_Factory::OpenAL_Factory(bool doSetup)
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

OpenAL_Factory::~OpenAL_Factory()
{
  // Deinitialize sound system
  if(didSetup)
    {
      alcMakeContextCurrent(NULL);
      if(Context) alcDestroyContext(Context);
      if(Device) alcCloseDevice(Device);
    }
}

// ---- OpenAL_Sound ----

void OpenAL_Sound::play()
{
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

void OpenAL_Sound::setPos(float x, float y, float z)
{
  alSource3f(inst, AL_POSITION, x, y, z);
  checkALError("setting position");
}

void OpenAL_Sound::setRepeat(bool rep)
{
  alSourcei(inst, AL_LOOPING, rep?AL_TRUE:AL_FALSE);
}

SoundPtr OpenAL_Sound::clone() const
{
  return SoundPtr(new OpenAL_Sound(bufferID, refCnt));
}

// Constructor used for cloned sounds
OpenAL_Sound::OpenAL_Sound(ALuint buf, int *ref)
  : refCnt(ref), bufferID(buf)
{
  // Increase the reference count
  assert(ref != NULL);
  *refCnt++;

  // Create a source
  alGenSources(1, &inst);
  alSourcei(inst, AL_BUFFER, bufferID);
}

// Constructor used for original (non-cloned) sounds
OpenAL_Sound::OpenAL_Sound(SampleSourcePtr input)
{
  // Get the format
  int fmt, rate;
  getALFormat(input, fmt, rate);

  // Set up the OpenAL buffer
  alGenBuffers(1, &bufferID);
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
      Mangle::Stream::BufferStream buf(input);

      // Then copy that into OpenAL
      alBufferData(bufferID, fmt, buf.getPtr(), buf.size(), rate);
    }

  checkALError("loading sound buffer");

  // Create a source
  alGenSources(1, &inst);
  alSourcei(inst, AL_BUFFER, bufferID);

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
      delete refCnt;
    }
}
