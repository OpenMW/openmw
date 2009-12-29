#ifndef MANGLE_SOUND_OUTPUT_H
#define MANGLE_SOUND_OUTPUT_H

#include <string>
#include "source.h"

#include "../stream/stream.h"

namespace Mangle {
namespace Sound {

/// Abstract interface for a single playable sound
/** This class represents one sound outlet, which may be played,
    stopped, paused and so on.

    Sound instances are created from the SoundFactory class. Sounds
    may be connected to a SampleSource or read directly from a file,
    and they may support 3d sounds, looping and other features
    depending on the capabilities of the backend system.
*/
class Sound
{
 public:
  /// Play or resume the sound
  virtual void play() = 0;

  /// Stop the sound
  virtual void stop() = 0;

  /// Pause the sound, may be resumed later
  virtual void pause() = 0;

  /// Check if the sound is still playing
  virtual bool isPlaying() = 0;

  /// Set the volume. The parameter must be between 0.0 and 1.0.
  virtual void setVolume(float) = 0;

  /// Set the position. May not have any effect on 2D sounds.
  virtual void setPos(float x, float y, float z) = 0;

  /// Virtual destructor
  virtual ~Sound() {}
};

/// Factory interface for creating Sound objects
/** The SoundFactory is the main entry point to a given sound output
    system. It is used to create Sound objects, which may be connected
    to a sound file or stream, and which may be individually played,
    paused, and so on.

    The class also contains a set of public bools which describe the
    capabilities the particular system. These should be set by
    implementations (base classes) in their respective constructors.
 */
class SoundFactory
{
 public:
  /// Virtual destructor
  virtual ~SoundFactory() {}

  /** @brief If set to true, you should call update() regularly (every frame
      or so) on this sound manager. If false, update() should not be
      called.
  */
  bool needsUpdate;

  /** @brief true if 3D functions are available. If false, all use of
      3D sounds and calls to setPos / setListenerPos will result in
      undefined behavior.
  */
  bool has3D;

  /** @brief true if 'repeat' and 'stream' can be used simultaneously.
      If false, repeating a streamed sound will give undefined
      behavior.
  */
  bool canRepeatStream;

  /// true if we can load sounds directly from file (containing encoded data)
  bool canLoadFile;

  /// If true, we can lound sound files from a Stream (containing encoded data)
  bool canLoadStream;

  /// true if we can load sounds from a SampleSource (containing raw data)
  bool canLoadSource;

  /**
     @brief Load a sound from a sample source. Only valid if
     canLoadSource is true.

     This function loads a sound from a given stream as defined by
     SampleSource.

     @param input the input source
     @param stream true if the file should be streamed.
            Implementations may use this for optimizing playback of
            large files, but they are not required to.
     @return a new Sound object
  */
  virtual Sound *load(SampleSource *input, bool stream=false) = 0;

  /**
     @brief Load a sound file from stream. Only valid if canLoadStream
     is true.

     @param input audio file stream
     @param stream true if the file should be streamed
     @see load(InputSource*,bool)
  */
  virtual Sound *load(Stream::Stream *input, bool stream=false) = 0;

  /**
     @brief Load a sound directly from file. Only valid if canLoadFile
     is true.

     @param file filename
     @param stream true if the file should be streamed
     @see load(InputSource*,bool)
  */
  virtual Sound *load(const std::string &file, bool stream=false) = 0;

  /// Call this every frame if needsUpdate is true
  /**
     This should be called regularly (about every frame in a normal
     game setting.) Implementions may use this for filling streaming
     buffers and similar tasks. Implementations that do not need this
     should set needsUpdate to false.
  */
  virtual void update() = 0;

  /// Set listener position (coordinates, front and up vectors)
  /**
     Only valid if has3D is true.

     @param x,y,z listener position
     @param fx,fy,fz listener's looking direction
     @param ux,uy,uz listener's up direction
   */
  virtual void setListenerPos(float x, float y, float z,
                              float fx, float fy, float fz,
                              float ux, float uy, float uz) = 0;
};

}} // Namespaces

#endif
