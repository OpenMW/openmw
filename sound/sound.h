#ifndef MANGLE_SOUND_SOUND_H
#define MANGLE_SOUND_SOUND_H

#include <string>
#include "input.h"

#include "../stream/input.h"

namespace Mangle {
namespace Sound {

/// Abstract interface for sound instances
/** This class represents one sound instance, which may be played,
    stopped, paused and so on. Instances are created from the Sound
    class. All instances must be terminated manually using the drop()
    function when they are no longer in use.
*/
class Instance
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

  /// Kill the current object
  virtual void drop() = 0;

  /// Virtual destructor
  virtual ~Instance() {}
};

/// Abstract interface for sound files or sources
/** This class acts as a factory for sound Instance objects.
    Implementations may choose to store shared sound buffers or other
    optimizations in subclasses of Sound. Objects of this class are
    created through the Manager class. All objects of this class
    should be terminated manually using the drop() function when they
    are no longer in use.
*/
class Sound
{
 public:
  /**
     @brief Create an instance of this sound

     See also the capability flags in the Manager class.

     @param is3d true if this the sound is to be 3d enabled
     @param repeat true if the sound should loop
     @return new Instance object
  */
  virtual Instance *getInstance(bool is3d, bool repeat) = 0;

  // Some prefab functions

  /// Shortcut for creating 3D instances
  Instance *get3D(bool loop=false)
    { return getInstance(true, loop); }
  /// Shortcut for creating 2D instances
  Instance *get2D(bool loop=false)
    { return getInstance(false, loop); }

  /// Kill the current object
  virtual void drop() = 0;

  /// Virtual destructor
  virtual ~Sound() {}
};

/// Abstract interface for the main sound manager class
/** The sound manager is used to load sound files and is a factory for
    Sound objects. It is the main entry point to a given sound system
    implementation.

    The class also contains a set of public bools which describe the
    capabilities the particular system. These should be set by
    implementations (base classes) in their respective constructors.
 */
class Manager
{
 public:
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

  /// true if we can load sounds directly from file
  bool canLoadFile;

  /// true if we can load sounds from an InputSource
  bool canLoadSource;

  /// If true, we can lound sound files from a Stream
  bool canLoadStream;

  /**
     @brief Load a sound from an input source. Only valid if
     canLoadSource is true.

     This function loads a sound from a given stream as defined by
     InputSource and InputStream. The InputSource and all streams
     created from it will be dropped when drop() is called on the
     owning sound / instance.

     @param input the input source
     @param stream true if the file should be streamed.
            Implementations may use this for optimizing playback of
            large files, but they are not required to.
     @return a new Sound object
  */
  virtual Sound *load(InputSource *input, bool stream=false) = 0;

  /**
     @brief Load a sound directly from file. Only valid if canLoadStream
     is true.

     @param input audio file stream
     @param stream true if the file should be streamed
     @see load(InputSource*,bool)
  */
  virtual Sound *load(Stream::InputStream *input, bool stream=false) = 0;

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
     Update function that should be called regularly (about every
     frame in a normal game setting.) Implementions may use this to
     fill streaming buffers and similar.  Implementations that do not
     need this should set needsUpdate to false.
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

  /// Virtual destructor
  virtual ~Manager() {}
};

}} // Namespaces

#endif
