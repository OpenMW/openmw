#ifndef MANGLE_VFS_H
#define MANGLE_VFS_H

#include "../stream/stream.h"
#include <string>
#include <vector>

namespace Mangle {
namespace VFS {

/// Generic file info structure
struct FileInfo
{
  /// Full name, including path
  std::string name;
  
  /// Base name, not including path
  std::string basename;

  /// Is this a directory?
  bool isDir;

  /// File size
  size_t size;
  
  /// Last modification date
  time_t time;
};

typedef std::vector<FileInfo> FileInfoList;

typedef boost::shared_ptr<FileInfo> FileInfoPtr;
typedef boost::shared_ptr<FileInfoList> FileInfoListPtr;

/** An interface to any file system or other provider of named data
    streams
*/
class VFS
{
 public:
  // Feature options. These should be set in the constructor.

  /// If true, the list() function work
  bool hasList;

  /// If true, the find() function work
  bool hasFind;

  /// If true, the file system is case sensitive
  bool isCaseSensitive;

  /// Virtual destructor
  virtual ~VFS() {}

  /// Open a new data stream. Deleting the object (letting all the
  /// pointers to it go out of scope) should be enough to close it.
  virtual Stream::StreamPtr open(const std::string &name) = 0;

  /// Check for the existence of a file
  virtual bool isFile(const std::string &name) const = 0;

  /// Check for the existence of a directory
  virtual bool isDir(const std::string &name) const = 0;

  /// Get info about a single file
  virtual FileInfoPtr stat(const std::string &name) const = 0;

  /// List all entries in a given directory. A blank dir should be
  /// interpreted as a the root/current directory of the archive. If
  /// dirs is true, list directories instead of files.
  virtual FileInfoListPtr list(const std::string& dir = "",
                               bool recurse=true,
                               bool dirs=false) const = 0;

  /// Find files after a given pattern. Wildcards (*) are
  /// supported. Only valid if 'hasFind' is true. Don't implement your
  /// own pattern matching here if the backend doesn't support it
  /// natively; use a filter instead.
  virtual FileInfoListPtr find(const std::string& pattern,
                               bool recursive=true,
                               bool dirs=false) const = 0;
};

typedef boost::shared_ptr<VFS> VFSPtr;

}} // namespaces
#endif
