// This file is shared between several test programs
#include "vfs.h"
#include <assert.h>
#include <string.h>

#include "../../stream/servers/memory_stream.h"

using namespace Mangle::VFS;
using namespace Mangle::Stream;

class DummyVFS : public VFS
{
public:
  DummyVFS()
  {
    hasFind = false;
    hasList = true;
    isCaseSensitive = true;
  }

  // We only support opening 'file1' at the moment.
  StreamPtr open(const std::string &name)
  {
    assert(name == "file1");
    return StreamPtr(new MemoryStream("hello world", 11));
  }

  bool isFile(const std::string &name) const
  {
    return (name == "file1" ||
            name == "dir/file2");
  }

  bool isDir(const std::string &name) const
  {
    return name == "dir";
  }

  /// Get info about a single file
  FileInfoPtr stat(const std::string &name) const
  {
    FileInfoPtr fi(new FileInfo);
    fi->name = name;
    fi->time = 0;

    if(isFile(name))
      {
        if(name == "dir/file2")
          {
            fi->basename = "file2";
            fi->size = 2;
          }
        else
          {
            fi->basename = "file1";
            fi->size = 1;
          }
        fi->isDir = false;
      }
    else if(isDir(name))
      {
        fi->basename = "dir";
        fi->isDir = true;
        fi->size = 0;
      }
    else assert(0);

    return fi;
  }

  /// List all entries in a given directory. A blank dir should be
  /// interpreted as a the root/current directory of the archive. If
  /// dirs is true, list directories instead of files.
  virtual FileInfoListPtr list(const std::string& dir = "",
                               bool recurse=true,
                               bool dirs=false) const
  {
    assert(dir == "");

    FileInfoListPtr fl(new FileInfoList);

    FileInfo fi;

    if(!dirs)
      {
        fi.name = "file1";
        fi.basename = "file1";
        fi.isDir = false;
        fi.size = 1;
        fi.time = 0;
        fl->push_back(fi);

        if(recurse)
          {
            fi.name = "dir/file2";
            fi.basename = "file2";
            fi.size = 2;
            fl->push_back(fi);
          }
      }
    else
      {
        fi.name = "dir";
        fi.basename = "dir";
        fi.isDir = true;
        fi.size = 0;
        fi.time = 0;
        fl->push_back(fi);
      }
    return fl;
  }

  FileInfoListPtr find(const std::string& pattern,
                    bool recursive=true,
                    bool dirs=false) const
  { assert(0); }
};
