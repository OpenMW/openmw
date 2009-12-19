// This file is shared between several test programs
#include "vfs.h"
#include <assert.h>
#include <string.h>

#include "../../stream/tests/dummy_input.cpp"

using namespace Mangle::VFS;

class DummyVFS : public VFS
{
public:
  DummyVFS()
  {
    hasFind = false;
    isCaseSensitive = true;
  }

  // We only support opening 'file1' at the moment.
  Mangle::Stream::InputStream *open(const std::string &name)
  {
    assert(name == "file1");
    return new DummyInput();
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
  FileInfo stat(const std::string &name) const
  {
    FileInfo fi;
    fi.name = name;
    fi.time = 0;

    if(isFile(name))
      {
        if(name == "dir/file2")
          {
            fi.basename = "file2";
            fi.size = 2;
          }
        else
          {
            fi.basename = "file1";
            fi.size = 1;
          }
        fi.isDir = false;
      }
    else if(isDir(name))
      {
        fi.basename = "dir";
        fi.isDir = true;
        fi.size = 0;
      }
    else assert(0);

    return fi;
  }

  /// List all entries in a given directory. A blank dir should be
  /// interpreted as a the root/current directory of the archive. If
  /// dirs is true, list directories instead of files.
  virtual FileInfoList list(const std::string& dir = "",
                            bool recurse=true,
                            bool dirs=false) const
  {
    assert(dir == "");

    FileInfoList fl;

    FileInfo fi;

    if(!dirs)
      {
        fi.name = "file1";
        fi.basename = "file1";
        fi.isDir = false;
        fi.size = 1;
        fi.time = 0;
        fl.push_back(fi);

        if(recurse)
          {
            fi.name = "dir/file2";
            fi.basename = "file2";
            fi.size = 2;
            fl.push_back(fi);
          }
      }
    else
      {
        fi.name = "dir";
        fi.basename = "dir";
        fi.isDir = true;
        fi.size = 0;
        fi.time = 0;
        fl.push_back(fi);
      }
    return fl;
  }

  FileInfoList find(const std::string& pattern,
                    bool recursive=true,
                    bool dirs=false) const
  { assert(0); return FileInfoList(); }
};
