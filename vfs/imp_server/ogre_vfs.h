#ifndef MANGLE_VFS_OGRECLIENT_H
#define MANGLE_VFS_OGRECLIENT_H

#include <OgreArchive.h>

namespace Mangle {
namespace VFS {

/** @brief An interface into the OGRE VFS system.

    This class does not wrap a single Ogre::Archive, but rather the
    entire resource set available to Ogre. You can use this class to
    tap into all paths, Zip files, custom archives on so on that have
    been inserted into Ogre as resource locations.

    This class is currently a work in progres, it will not compile as
    it stands.

    This has been built and tested against OGRE 1.6.2. You might have
    to make your own modifications if you're working with newer (or
    older) versions.
 */
class OgreVFS : public VFS
{
 public:
  OgreVFS()
    {
      hasFind = true;
      isCaseSensitive = true;
    }

  /// Open a new data stream. Deleting the object should be enough to
  /// close it.
  virtual Stream::InputStream *open(const std::string &name);

  /// Check for the existence of a file
  virtual bool isFile(const std::string &name) const;

  /// Check for the existence of a directory
  virtual bool isDir(const std::string &name) const;

  /// Get info about a single file
  virtual FileInfo stat(const std::string &name) const;

  /// List all entries in a given directory. A blank dir should be
  /// interpreted as a the root/current directory of the archive. If
  /// dirs is true, list directories instead of files.
  virtual FileInfoList list(const std::string& dir = "",
                            bool recurse=true,
                            bool dirs=false) const;

  /// Find files after a given pattern. Wildcards (*) are
  /// supported. Only valid if 'hasFind' is true. Don't implement your
  /// own pattern matching here if the backend doesn't support it
  /// natively; use a filter instead (not implemented yet.)
  virtual FileInfoList find(const std::string& pattern,
                            bool recursive=true,
                            bool dirs=false) const;
};

}} // namespaces
#endif
