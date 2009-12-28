#ifndef MANGLE_VFS_PHYSFS_SERVER_H
#define MANGLE_VFS_PHYSFS_SERVER_H

#include "../vfs.h"
#include "../../stream/servers/phys_stream.h"

#include <physfs.h>
#include <assert.h>

namespace Mangle {
namespace VFS {

/** @brief An interface into the PhysFS virtual file system library

    You have to set up PhysFS on your own before using this class.
 */
class PhysVFS : public VFS
{
 public:
  PhysVFS()
    {
      hasList = true;
      hasFind = false;
      isCaseSensitive = true;
    }

  /// Open a new data stream. Deleting the object should be enough to
  /// close it.
  virtual Stream::Stream *open(const std::string &name)
    { return new Stream::PhysFile(PHYSFS_openRead(name.c_str())); }

  /// Check for the existence of a file
  virtual bool isFile(const std::string &name) const
    { return PHYSFS_exists(name.c_str()); }

  /// Checks for a directory
  virtual bool isDir(const std::string &name) const
    { return PHYSFS_isDirectory(name.c_str()); }

  /// This doesn't work
  virtual FileInfo stat(const std::string &name) const
    { assert(0); return FileInfo(); }

  virtual FileInfoList list(const std::string& dir = "",
                            bool recurse=true,
                            bool dirs=false) const
    {
      char **files = PHYSFS_enumerateFiles(dir.c_str());
      FileInfoList lst;

      // Add all teh files
      int i = 0;
      while(files[i] != NULL)
        {
          FileInfo fi;
          fi.name = files[i];
          fi.isDir = false;

          lst.push_back(fi);
        }
      return lst;
    }

  virtual FileInfoList find(const std::string& pattern,
                            bool recursive=true,
                            bool dirs=false) const
    { assert(0); }
};

}} // namespaces
#endif
