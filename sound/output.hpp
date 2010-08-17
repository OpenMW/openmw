#ifndef MANGLE_SOUND_OUTPUT_H
#define MANGLE_SOUND_OUTPUT_H

#include <string>
#include <assert.h>

#include "source.hpp"
#include "../stream/stream.hpp"

namespace Mangle {
namespace Sound {

/// Abstract interface for a single playable sound
/** This class represents one sound outlet, which may be played,
    stopped, paused and so on.

    Sound instances are created from the SoundFactory class. Sounds
    may be connected to a SampleSource or read directly from a file,
    and they may support 3d sounds, looping and other features
    depending on the capabilities of the backend system.

    To create multiple instances of one sound, it is recommended to
    'clone' an existing instance instead of reloading it from
    file. Cloned sounds will often (depending on the back-end) use
    less memory due to shared buffers.
*/
class Sound;
typedef boost::shared_ptr<Sound> SoundPtr;

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
  virtual bool isPlaying() const = 0;

  /// Set the volume. The parameter must be between 0.0 and 1.0.
  virtual void setVolume(float) = 0;

  /// Set left/right pan. -1.0 is left, 0.0 is center and 1.0 is right.
  virtual void setPan(float) = 0;

  /// Set pitch (1.0 is normal speed)
  virtual void setPitch(float) = 0;

  /// Set range factors for 3D sounds. The meaning of the fields
  /// depend on implementation.
  virtual void setRange(float a, float b=0.0, float c=0.0) = 0;

  /// Set the position. May not work with all backends.
  virtual void setPos(float x, float y, float z) = 0;

  /// Set loop mode
  virtual void setRepeat(bool) = 0;

  /// Set streaming mode.
  /** This may be used by implementations to optimize for very large
      files. If streaming mode is off (default), most implementations
      will load the entire file into memory before starting playback.
   */
  virtual void setStreaming(bool) = 0;

  /// Create a new instance of this sound.
  /** Playback status is not cloned, only the sound data
      itself. Back-ends can use this as a means of sharing data and
      saving memory. */
  virtual SoundPtr clone() = 0;

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
  virtual SoundPtr loadRaw(SampleSourcePtr input) = 0;

  /**
     @brief Load a sound file from stream. Only valid if canLoadStream
     is true.

     @param input audio file stream
     @param stream true if the file should be streamed
     @see load(InputSource*,bool)
  */
  virtual SoundPtr load(Stream::StreamPtr input) = 0;

  /**
     @brief Load a sound directly from file. Only valid if canLoadFile
     is true.

     @param file filename
     @param stream true if the file should be streamed
     @see load(InputSource*,bool)
  */
  virtual SoundPtr load(const std::string &file) = 0;

  /// Call this every frame if needsUpdate is true
  /**
     This should be called regularly (about every frame in a normal
     game setting.) Implementions may use this for filling streaming
     buffers and similar tasks. Implementations that do not need this
     should set needsUpdate to false.
  */
  virtual void update() { assert(0); }

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

typedef boost::shared_ptr<SoundFactory> SoundFactoryPtr;

}} // Namespaces

#endif
