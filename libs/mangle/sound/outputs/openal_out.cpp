#include "openal_out.hpp"
#include <assert.h>
#include <stdexcept>

#include "../../stream/filters/buffer_stream.hpp"

#ifdef _WIN32
#include <al.h>
#include <alc.h>
#elif defined(__APPLE__)
#include <OpenAL/alc.h>
#include <OpenAL/al.h>
#else
#include <AL/al.h>
#include <AL/alc.h>
#endif

using namespace Mangle::Sound;

// ---- Helper functions and classes ----

// Static buffer used to shuffle sound data from the input into
// OpenAL. The data is only stored temporarily and then immediately
// shuffled off to the library. This is not thread safe, but it works
// fine with multiple sounds in one thread. It could be made thread
// safe simply by using thread local storage.
const size_t BSIZE = 32*1024;
static char tmp_buffer[BSIZE];

// Number of buffers used (per sound) for streaming sounds. Each
// buffer is of size BSIZE. Increasing this will make streaming sounds
// more fault tolerant against temporary lapses in call to update(),
// but will also increase memory usage.
// This was changed from 4 to 150 for an estimated 30 seconds tolerance.
// At some point we should replace it with a more multithreading-ish
// solution.
const int STREAM_BUF_NUM = 150;

static void fail(const std::string &msg)
{ throw std::runtime_error("OpenAL exception: " + msg); }

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
  boost::int32_t rate_, ch, bits;
  inp->getInfo(&rate_, &ch, &bits);
  rate = rate_;

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
class Mangle::Sound::OpenAL_Sound : public Sound
{
  ALuint inst;

  // Buffers. Only the first is used for non-streaming sounds.
  ALuint bufferID[STREAM_BUF_NUM];

  // Number of buffers used
  int bufNum;

  // Parameters used for filling buffers
  int fmt, rate;

  // Poor mans reference counting. Might improve this later. When
  // NULL, the buffer has not been set up yet.
  int *refCnt;

  bool streaming;

  // Input stream
  SampleSourcePtr input;

  OpenAL_Factory *owner;
  bool ownerAlive;

  // Used for streamed sound list
  OpenAL_Sound *next, *prev;

  void setupBuffer();

  // Fill data into the given buffer and queue it, if there is any
  // data left to queue. Assumes the buffer is already unqueued, if
  // necessary.
  void queueBuffer(ALuint buf)
  {
    // If there is no more data, do nothing
    if(!input) return;
    if(input->eof())
      {
        input.reset();
        return;
      }

    // Get some new data
    size_t bytes = input->read(tmp_buffer, BSIZE);
    if(bytes == 0)
      {
        input.reset();
        return;
      }

    // Move data into the OpenAL buffer
    alBufferData(buf, fmt, tmp_buffer, bytes, rate);
    // Queue it
    alSourceQueueBuffers(inst, 1, &buf);
    checkALError("Queueing buffer data");
  }

 public:
  /// Read samples from the given input buffer
  OpenAL_Sound(SampleSourcePtr input, OpenAL_Factory *fact);

  /// Play an existing buffer, with a given ref counter. Used
  /// internally for cloning.
  OpenAL_Sound(ALuint buf, int *ref, OpenAL_Factory *fact);

  ~OpenAL_Sound();

  // Must be called regularly on streamed sounds
  void update()
  {
    if(!streaming) return;
    if(!input) return;

    // Get the number of processed buffers
    ALint count;
    alGetSourcei(inst, AL_BUFFERS_PROCESSED, &count);
    checkALError("getting number of unprocessed buffers");

    for(int i=0; i<count; i++)
      {
        ALuint buf;
        // Unqueue one of the processed buffer
        alSourceUnqueueBuffers(inst, 1, &buf);

        // Then reload it with data (if any) and queue it up
        queueBuffer(buf);
      }
  }

  void play();
  void stop();
  void pause();
  bool isPlaying() const;
  void setVolume(float);
  void setPos(float x, float y, float z);
  void setPitch(float);
  void setRepeat(bool);

  void notifyOwnerDeath()
  { ownerAlive = false; }

  // We can enable streaming, but never disable it.
  void setStreaming(bool s)
  { if(s) streaming = true; }

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
  return SoundPtr(new OpenAL_Sound(input, this));
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

void OpenAL_Factory::update()
{
  // Loop through all streaming sounds and update them
  StreamList::iterator it = streaming.begin();
  for(;it != streaming.end(); it++)
    (*it)->update();
}

void OpenAL_Factory::notifyStreaming(OpenAL_Sound *snd)
{
  // Add the sound to the streaming list
  streaming.push_back(snd);
}

void OpenAL_Factory::notifyDelete(OpenAL_Sound *snd)
{
  // Remove the sound from the stream list
  streaming.remove(snd);
}

OpenAL_Factory::~OpenAL_Factory()
{
  // Notify remaining streamed sounds that we're dying
  StreamList::iterator it = streaming.begin();
  for(;it != streaming.end(); it++)
    (*it)->notifyOwnerDeath();

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
  return SoundPtr(new OpenAL_Sound(bufferID[0], refCnt, owner));
}

// Constructor used for cloned sounds
OpenAL_Sound::OpenAL_Sound(ALuint buf, int *ref, OpenAL_Factory *fact)
  : refCnt(ref), streaming(false), owner(fact), ownerAlive(false)
{
  // Increase the reference count
  assert(ref != NULL);
  (*refCnt)++;

  // Set up buffer
  bufferID[0] = buf;
  bufNum = 1;

  // Create a source
  alGenSources(1, &inst);
  checkALError("creating instance (clone)");
  alSourcei(inst, AL_BUFFER, bufferID[0]);
  checkALError("assigning buffer (clone)");
}

// Constructor used for original (non-cloned) sounds
OpenAL_Sound::OpenAL_Sound(SampleSourcePtr _input, OpenAL_Factory *fact)
  : refCnt(NULL), streaming(false), input(_input), owner(fact), ownerAlive(false)
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
  getALFormat(input, fmt, rate);

  // Create a cheap reference counter for the buffer
  refCnt = new int;
  *refCnt = 1;

  if(streaming) bufNum = STREAM_BUF_NUM;
  else bufNum = 1;

  // Set up the OpenAL buffer(s)
  alGenBuffers(bufNum, bufferID);
  checkALError("generating buffer(s)");
  assert(bufferID[0] != 0);

  // STREAMING.
  if(streaming)
    {
      // Just queue all the buffers with data and exit. queueBuffer()
      // will work correctly also in the case where there is not
      // enough data to fill all the buffers.
      for(int i=0; i<bufNum; i++)
        queueBuffer(bufferID[i]);

      // Notify the manager what we're doing
      owner->notifyStreaming(this);
      ownerAlive = true;

      return;
    }

  // NON-STREAMING. We have to load all the data and shove it into the
  // buffer.

  // Does the stream support pointer operations?
  if(input->hasPtr)
    {
      // If so, we can read the data directly from the stream
      alBufferData(bufferID[0], fmt, input->getPtr(), input->size(), rate);
    }
  else
    {
      // Read the entire stream into a temporary buffer first
      Mangle::Stream::BufferStream buf(input, 128*1024);

      // Then copy that into OpenAL
      alBufferData(bufferID[0], fmt, buf.getPtr(), buf.size(), rate);
    }
  checkALError("loading sound data");

  // We're done with the input stream, release the pointer
  input.reset();

  alSourcei(inst, AL_BUFFER, bufferID[0]);
  checkALError("assigning buffer");
}

OpenAL_Sound::~OpenAL_Sound()
{
  // Stop
  alSourceStop(inst);

  // Return sound
  alDeleteSources(1, &inst);

  // Notify the factory that we quit. You will hear from our union
  // rep. The bool check is to handle cases where the manager goes out
  // of scope before the sounds do. In that case, don't try to contact
  // the factory.
  if(ownerAlive)
    owner->notifyDelete(this);

  // Decrease the reference counter
  if((-- (*refCnt)) == 0)
    {
      // We're the last owner. Delete the buffer(s) and the counter
      // itself.
      alDeleteBuffers(bufNum, bufferID);
      checkALError("deleting buffer");
      delete refCnt;
    }
}
