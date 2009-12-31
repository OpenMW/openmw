#ifndef MANGLE_STREAM_OGRECLIENT_H
#define MANGLE_STREAM_OGRECLIENT_H

#include <OgreDataStream.h>
#include <assert.h>
#include "../stream.h"

namespace Mangle {
namespace Stream {

/** An OGRE DataStream that wraps a Mangle::Stream input.

    This has been built and tested against OGRE 1.6.2. You might have
    to make your own modifications if you're working with newer (or
    older) versions.
 */
class MangleDataStream : public Ogre::DataStream
{
  StreamPtr inp;

  void init()
    {
      // Get the size, if possible
      if(inp->hasSize)
        mSize = inp->size();
    }

 public:
  /// Constructor without name
  MangleDataStream(StreamPtr _inp)
    : inp(_inp) { init(); }

  /// Constructor for a named data stream
  MangleDataStream(const Ogre::String &name, StreamPtr _inp)
    : inp(_inp), Ogre::DataStream(name) { init(); }

  // Only implement the DataStream functions we have to implement

  size_t read(void *buf, size_t count)
    { return inp->read(buf,count); }

  void skip(long count)
    {
      assert(inp->isSeekable && inp->hasPosition);
      inp->seek(inp->tell() + count);
    }

  void seek(size_t pos)
    { assert(inp->isSeekable); inp->seek(pos); }

  size_t tell() const
    { assert(inp->hasPosition); return inp->tell(); }

  bool eof() const { return inp->eof(); }

  /// Does nothing
  void close() {}
};

}} // namespaces
#endif
