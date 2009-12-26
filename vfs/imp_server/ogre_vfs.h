#ifndef MANGLE_VFS_OGRESERVER_H
#define MANGLE_VFS_OGRESERVER_H

#include "../vfs.h"
#include <OgreResourceGroupManager.h>

namespace Mangle {
namespace VFS {

/** @brief An interface into the OGRE VFS system.

    This class does NOT wrap a single Ogre::Archive, but rather an
    entire resource group in Ogre. You can use this class to tap into
    all paths, Zip files, custom archives on so on that have been
    inserted into Ogre as resource locations.

    This has been built and tested against OGRE 1.6.2. You might have
    to make your own modifications if you're working with newer (or
    older) versions.
 */
class OgreVFS : public VFS
{
  std::string group;
  Ogre::ResourceGroupManager *gm;

 public:
  /** @brief Constructor

      OGRE must be initialized (ie. you must have created an
      Ogre::Root object somewhere) before calling this.

      @param group Optional resource group name. If none is given,
      OGRE's default (or 'World') resource group is used.
   */
  OgreVFS(const std::string &_group = "");

  /// Open a new data stream. Deleting the object should be enough to
  /// close it.
  virtual Stream::InputStream *open(const std::string &name);

  /// Check for the existence of a file
  virtual bool isFile(const std::string &name) const
    { return gm->resourceExists(group, name); }

  /// This doesn't work, always returns false.
  virtual bool isDir(const std::string &name) const
    { return false; }

  /// This doesn't work.
  virtual FileInfo stat(const std::string &name) const
    { return FileInfo(); }

  /// List all entries in a given directory. A blank dir should be
  /// interpreted as a the root/current directory of the archive. If
  /// dirs is true, list directories instead of files. OGRE note: The
  /// ogre resource systemd does not support recursive listing of
  /// files. We might make a separate filter for this later.
  virtual FileInfoList list(const std::string& dir = "",
                            bool recurse=true,
                            bool dirs=false) const;

  /// Find files after a given pattern. Wildcards (*) are
  /// supported.
  virtual FileInfoList find(const std::string& pattern,
                            bool recursive=true,
                            bool dirs=false) const;
};

}} // namespaces
#endif
