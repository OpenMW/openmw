#ifndef MANGLE_STREAM_OGRESERVER_H
#define MANGLE_STREAM_OGRESERVER_H

#include <OgreDataStream.h>

namespace Mangle {
namespace Stream {

/** A Stream wrapping an OGRE DataStream.

    This has been built and tested against OGRE 1.6.2. You might have
    to make your own modifications if you're working with newer (or
    older) versions.
 */
class OgreStream : public InputStream
{
  Ogre::DataStreamPtr inp;

 public:
 OgreStream(Ogre::DataStreamPtr _inp) : inp(_inp)
  {
    isSeekable = true;
    hasPosition = true;
    hasSize = true;
  }

  size_t read(void *buf, size_t count) { return inp->read(buf,count); }
  void seek(size_t pos) { inp->seek(pos); }
  size_t tell() const { return inp->tell(); }
  size_t size() const { return inp->size(); }
  bool eof() const { return inp->eof(); }
};

}} // namespaces
#endif
