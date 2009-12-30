#include "openal_out.h"
#include <assert.h>

using namespace Mangle::Sound;

// ---- Helper functions and classes ----

class OpenAL_Exception : public std::exception
{
  std::string msg;

 public:

  OpenAL_Exception(const std::string &m) : msg(m) {}
  ~OpenAL_Exception() throw() {}
  virtual const char* what() const throw() { return msg.c_str(); }
};

static void fail(const std::string &msg)
{
  throw OpenAL_Exception("OpenAL exception: " + msg);
}

static void checkALError(const std::string &msg)
{
  ALenum err = alGetError();
  if(err != AL_NO_ERROR)
    fail("\"" + std::string(alGetString(err)) + "\" while " + msg);
}

static void getALFormat(InputStream *inp, int &fmt, int &rate)
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

bool OpenAL_Sound::isPlaying()
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

OpenAL_Sound::OpenAL_Sound(SampleSource *input)
{
  // Get the format
  int fmt, rate;
  getALFormat(inp, fmt, rate);

  // Read the entire stream into a buffer
  BufferStream buf(input);

  // Move the data into OpenAL
  alGenBuffers(1, &bufferID);
  assert(bufferID != 0);
  alBufferData(bufferID, fmt, &buf.getPtr(), buf.size(), rate);
  checkALError("loading sound buffer");

  // Create a source
  alGenSources(1, &inst);
  alSourcei(inst, AL_BUFFER, buf);
}

OpenAL_Sound::~OpenAL_Sound()
{
  // Stop
  alSourceStop(inst);

  // Return sound
  alDeleteSources(1, &inst);

  // Delete buffer
  alDeleteBuffers(1, &bufferID);
}
