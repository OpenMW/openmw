#ifndef GOOI_SOUND_AUDIERE_H
#define GOOI_SOUND_AUDIERE_H

#include "../sound.h"

#include <assert.h>
#include <audiere.h>

namespace GOOI {
namespace Sound {

/// Implementation of Sound::Manager for Audiere
class AudiereManager : public Manager
{
  audiere::AudioDevicePtr device;

 public:
  AudiereManager();

  virtual Sound *load(const std::string &file, bool stream=false);

  /// disabled
  virtual Sound *load(InputSource *input, bool stream=false)
    { assert(0); }
  /// disabled
  virtual void update() { assert(0); }
  /// disabled
  virtual void setListenerPos(float x, float y, float z,
                              float fx, float fy, float fz,
                              float ux, float uy, float uz)
    { assert(0); };
};

/// Audiere Sound implementation
class AudiereSound : public Sound
{
  audiere::AudioDevicePtr device;
  audiere::SampleSourcePtr sample;
  audiere::SampleBufferPtr buf;

  bool stream;

 public:
  virtual Instance *getInstance(bool is3d, bool repeat);
  virtual void drop()
    { delete this; }

  AudiereSound(const std::string &file, audiere::AudioDevicePtr device,
               bool stream);
};

/// Audiere Instance implementation
class AudiereInstance : public Instance
{
  audiere::OutputStreamPtr sound;

 public:
  virtual void play();
  virtual void stop();
  virtual void pause();
  virtual bool isPlaying();
  virtual void setVolume(float);
  /// disabled
  virtual void setPos(float x, float y, float z)
    { assert(0); }
  virtual void drop()
    { delete this; }

  AudiereInstance(audiere::OutputStreamPtr);
};

}} // Namespace
#endif
