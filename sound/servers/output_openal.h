#ifndef MANGLE_SOUND_OPENAL_H
#define MANGLE_SOUND_OPENAL_H

#include "../sound.h"

#include <AL/al.h>
#include <AL/alc.h>
#include <list>

namespace Mangle {
namespace Sound {

class OpenAL_Stream_Instance;

/// OpenAL implementation of Manager
class OpenAL_Manager : public Manager
{
public:
  // List of all streaming sounds - these need to be updated regularly
  typedef std::list<OpenAL_Stream_Instance*> LST;

  OpenAL_Manager();
  virtual ~OpenAL_Manager();

  LST::iterator add_stream(OpenAL_Stream_Instance*);
  void remove_stream(LST::iterator);

  virtual Sound *load(const std::string &file, bool stream=false);
  virtual Sound *load(Stream::Stream *input, bool stream=false);
  virtual Sound *load(InputSource* input, bool stream=false);
  virtual void update();
  virtual void setListenerPos(float x, float y, float z,
                              float fx, float fy, float fz,
                              float ux, float uy, float uz);

 private:
  ALCdevice  *Device;
  ALCcontext *Context;

  LST streaming;
};

/// OpenAL implementation of Sound
class OpenAL_Sound : public Sound
{
  InputSource *source;
  OpenAL_Manager *owner;
  bool stream;

  // Used for non-streaming files, contains the entire sound buffer if
  // non-zero
  ALuint bufferID;

 public:
  OpenAL_Sound(InputSource *src, OpenAL_Manager *own, bool str)
    : source(src), owner(own), stream(str), bufferID(0) {}
  ~OpenAL_Sound();

  virtual Instance *getInstance(bool is3d, bool repeat);
  void drop() { delete this; }
};

/// Shared parent class that holds an OpenAL sound instance. Just used
/// for shared functionality, has no setup or cleanup code.
class OpenAL_Instance_Base : public Instance
{
 protected:
  ALuint inst;

 public:
  void drop() { delete this; }
  virtual void play();
  virtual void stop();
  virtual void pause();
  virtual bool isPlaying();
  virtual void setVolume(float);
  virtual void setPos(float x, float y, float z);  
};

/// Non-streaming OpenAL-implementation of Instance. Uses a shared
/// sound buffer in OpenAL_Sound.
class OpenAL_Simple_Instance : public OpenAL_Instance_Base
{
 public:
  OpenAL_Simple_Instance(ALuint buf);
  ~OpenAL_Simple_Instance();
};

/// Streaming OpenAL-implementation of Instance.
class OpenAL_Stream_Instance : public OpenAL_Instance_Base
{
  // Since OpenAL streams have to be updated manually each frame, we
  // need to have a sufficiently large buffer so that we don't run out
  // of data in the mean time. Each instance will take around 512 Kb
  // of memory, independent of how large the file is.
  static const int BUFS = 4;
  static const int SIZE = 128*1024;

  // Buffers
  ALuint bufs[BUFS];

  // Sound format settings
  int rate, fmt;

  // Source of data
  InputStream *stream;

  OpenAL_Manager *owner;

  // List iterator, used for removing ourselves from the streaming
  // list when we're deleted.
  OpenAL_Manager::LST::iterator lit;

  // Load and queue a new buffer
  void queueBuffer(ALuint buffer);

public:
  OpenAL_Stream_Instance(InputStream*, OpenAL_Manager*);
  ~OpenAL_Stream_Instance();

  void update();
};

}} // namespaces
#endif
