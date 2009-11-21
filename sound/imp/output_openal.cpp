#include "output_openal.h"
#include <assert.h>

#include <vector>

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


// ---- Manager ----

OpenAL_Manager::OpenAL_Manager()
  : Context(NULL), Device(NULL)
{
  needsUpdate = true;
  has3D = true;
  canRepeatStream = false;
  canLoadFile = false;
  canLoadSource = true;

  // Set up sound system
  Device = alcOpenDevice(NULL);
  Context = alcCreateContext(Device, NULL);

  if(!Device || !Context)
    fail("Failed to initialize context or device");

  alcMakeContextCurrent(Context);
}

OpenAL_Manager::~OpenAL_Manager()
{
  // Deinitialize sound system
  alcMakeContextCurrent(NULL);
  if(Context) alcDestroyContext(Context);
  if(Device) alcCloseDevice(Device);
}

Sound *OpenAL_Manager::load(const std::string &file, bool stream)
{ assert(0 && "OpenAL cannot decode files"); }

Sound *OpenAL_Manager::load(InputSource *source, bool stream)
{ return new OpenAL_Sound(source, this, stream); }

void OpenAL_Manager::update()
{
  // Loop through all the streaming sounds and update them
  LST::iterator it, next;
  for(it = streaming.begin();
      it != streaming.end();
      it++)
    {
      (*it)->update();
    }
}

void OpenAL_Manager::setListenerPos(float x, float y, float z,
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
  checkALError("setting listener position");
}

OpenAL_Manager::LST::iterator OpenAL_Manager::add_stream(OpenAL_Stream_Instance* inst)
{
  streaming.push_front(inst);
  return streaming.begin();
}

void OpenAL_Manager::remove_stream(LST::iterator it)
{
  streaming.erase(it);
}


// ---- Sound ----

OpenAL_Sound::~OpenAL_Sound()
{
  // Kill the input source
  if(source) source->drop();

  // And any allocated buffers
  if(bufferID)
    alDeleteBuffers(1, &bufferID);
}

Instance *OpenAL_Sound::getInstance(bool is3d, bool repeat)
{
  assert((!repeat || !stream) && "OpenAL implementation does not support looping streams");

  if(stream)
    return new OpenAL_Stream_Instance(source->getStream(), owner);

  // Load the buffer if it hasn't been done already
  if(bufferID == 0)
    {
      // Get an input stream and load the file from it
      InputStream *inp = source->getStream();

      std::vector<unsigned char> buffer;

      // Add 32 kb at each increment
      const int ADD = 32*1024;

      // Fill the buffer. We increase the buffer until it's large
      // enough to fit all the data.
      while(true)
        {
          // Increase the buffer
          int oldlen = buffer.size();
          buffer.resize(oldlen+ADD);

          // Read the data
          size_t len = inp->getData(&buffer[oldlen], ADD);

          // If we read less than requested, we're done.
          if(len < ADD)
            {
              // Downsize the buffer to the right size
              buffer.resize(oldlen+len);
              break;
            }
        }

      // Get the format
      int fmt, rate;
      getALFormat(inp, fmt, rate);

      // We don't need the file anymore
      inp->drop();
      source->drop();
      source = NULL;

      // Move the data into OpenAL
      alGenBuffers(1, &bufferID);
      alBufferData(bufferID, fmt, &buffer[0], buffer.size(), rate);
      checkALError("loading sound buffer");
    } // End of buffer loading

  // At this point, the file data has been loaded into the buffer
  // in 'bufferID', and we should be ready to go.
  assert(bufferID != 0);

  return new OpenAL_Simple_Instance(bufferID);
}


// ---- OpenAL_Instance_Base ----

void OpenAL_Instance_Base::play()
{
  alSourcePlay(inst);
  checkALError("starting playback");
}

void OpenAL_Instance_Base::stop()
{
  alSourceStop(inst);
  checkALError("stopping");
}

void OpenAL_Instance_Base::pause()
{
  alSourcePause(inst);
  checkALError("pausing");
}

bool OpenAL_Instance_Base::isPlaying()
{
  ALint state;
  alGetSourcei(inst, AL_SOURCE_STATE, &state);

  return state == AL_PLAYING;
}

void OpenAL_Instance_Base::setVolume(float volume)
{
  if(volume > 1.0) volume = 1.0;
  if(volume < 0.0) volume = 0.0;
  alSourcef(inst, AL_GAIN, volume);
  checkALError("setting volume");
}

void OpenAL_Instance_Base::setPos(float x, float y, float z)
{
  alSource3f(inst, AL_POSITION, x, y, z);
  checkALError("setting position");
}


// ---- OpenAL_Simple_Instance ----

OpenAL_Simple_Instance::OpenAL_Simple_Instance(ALuint buf)
{
  // Create instance and associate buffer
  alGenSources(1, &inst);
  alSourcei(inst, AL_BUFFER, buf);
}

OpenAL_Simple_Instance::~OpenAL_Simple_Instance()
{
  // Stop
  alSourceStop(inst);

  // Return sound
  alDeleteSources(1, &inst);
}


// ---- OpenAL_Stream_Instance ----

OpenAL_Stream_Instance::OpenAL_Stream_Instance(InputStream *_stream,
                                               OpenAL_Manager *_owner)
  : stream(_stream), owner(_owner)
{
  // Deduce the file format from the stream info
  getALFormat(stream, fmt, rate);

  // Create the buffers and the sound instance
  alGenBuffers(BUFS, bufs);
  alGenSources(1, &inst);

  checkALError("initializing");

  // Fill the buffers and que them
  for(int i=0; i<BUFS; i++)
    queueBuffer(bufs[i]);

  checkALError("buffering initial data");

  // Add ourselves to the stream list
  lit = owner->add_stream(this);
}

void OpenAL_Stream_Instance::queueBuffer(ALuint bId)
{
  char buf[SIZE];

  // Get the data
  int len = stream->getData(buf, SIZE);
  if(len == 0)
    return;

  // .. and stash it
  alBufferData(bId, fmt, buf, len, rate);
  alSourceQueueBuffers(inst, 1, &bId);
}

OpenAL_Stream_Instance::~OpenAL_Stream_Instance()
{
  // Remove ourselves from streaming list
  owner->remove_stream(lit);

  // Stop
  alSourceStop(inst);

  // Kill the input stream
  stream->drop();

  // Return sound
  alDeleteSources(1, &inst);

  // Delete buffers
  alDeleteBuffers(BUFS, bufs);
}

void OpenAL_Stream_Instance::update()
{
  ALint count;
  alGetSourcei(inst, AL_BUFFERS_PROCESSED, &count);

  for(int i = 0;i < count;i++)
    {
      // Unque a finished buffer
      ALuint bId;
      alSourceUnqueueBuffers(inst, 1, &bId);

      // Queue a new buffer
      queueBuffer(bId);
    }
}
