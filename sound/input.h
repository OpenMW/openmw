#ifndef MANGLE_SOUND_INPUT_H
#define MANGLE_SOUND_INPUT_H

#include <string>
#include <stdint.h>

namespace Mangle {
namespace Sound {

/// An abstract interface for a read-once stream of audio data.
/** All instances of this is created through InputSource. Objects
    should be manually deleted through a call to drop() when they are
    no longer needed.
*/
class InputStream
{
 public:
  /// Get the sample rate, number of channels, and bits per
  /// sample. NULL parameters are ignored.
  virtual void getInfo(int32_t *rate, int32_t *channels, int32_t *bits) = 0;

  /// Get decoded sound data from the stream.
  /** Stores 'length' bytes (or less) in the buffer pointed to by
      'output'. Returns the number of bytes written. The function will
      only return less than 'length' at the end of the stream. When
      the stream is empty, all subsequent calls will return zero.

      @param output where to store data
      @param length number of bytes to get
      @return number of bytes actually written
  */
  virtual uint32_t getData(void *output, uint32_t length) = 0;

  /// Kill this object
  virtual void drop() = 0;

  /// Virtual destructor
  virtual ~InputStream() {}
};

/// Abstract interface representing one sound source.
/** A sound source may represent one sound file or buffer, and is a
    factory for producing InputStream objects from that
    sound. Instances of this class are created by an InputManager. All
    instances should be deleted through drop() when they are no longer
    needed.
 */
class InputSource
{
 public:
  /// Create a stream from this sound
  virtual InputStream *getStream() = 0;

  /// Kill this object
  virtual void drop() = 0;

  /// Virtual destructor
  virtual ~InputSource() {}
};

/// Main interface to a sound decoder backend.
/** An input manager is a factory of InputSource objects.
 */
class InputManager
{
 public:
  /// Load a sound input source from file
  virtual InputSource *load(const std::string &file) = 0;

  /// Virtual destructor
  virtual ~InputManager() {}
};

}} // namespaces
#endif
