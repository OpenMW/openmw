#ifndef MANGLE_VFS_OGRECLIENT_H
#define MANGLE_VFS_OGRECLIENT_H

#include <OgreArchive.h>
#include <assert.h>
#include "wrapper.h"

namespace Mangle {
namespace VFS {

/** An OGRE Archive implementation that wraps a Mangle::VFS
    filesystem.

    This has been built and tested against OGRE 1.6.2. You might have
    to make your own modifications if you're working with newer (or
    older) versions.
 */
class MangleArchive : public Ogre::Archive, _Wrapper
{
 public:
  /// Constructor without name
  MangleArchive(VFS *vfs, const std::string &name,
                const std::string &archType = "Mangle",
                bool autoDel=false)
    : _Wrapper(vfs, autoDel), Ogre::Archive(name, archType) {}

  bool isCaseSensitive() const { return vfs->isCaseSensitive; }

  // These do nothing. You have to load / unload the archive manually.
  void load() {}
  void unload() {}

  bool exists(const Ogre::String& filename)
    { return vfs->isFile(filename); }

  time_t getModifiedTime(const Ogre::String& filename)
    { return vfs->stat(filename).time; }

  Ogre::DataStreamPtr open(const Ogre::String& filename) const;

  Ogre::StringVectorPtr list(bool recursive = true, bool dirs = false);
  Ogre::FileInfoListPtr listFileInfo(bool recursive = true, bool dirs = false);

  // Find functions will only work if vfs->hasFind is set.
  Ogre::StringVectorPtr find(const Ogre::String& pattern, bool recursive = true,
                             bool dirs = false);
  Ogre::FileInfoListPtr findFileInfo(const Ogre::String& pattern, 
                                     bool recursive = true,
                                     bool dirs = false);
};

}} // namespaces
#endif
