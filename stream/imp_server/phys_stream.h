#ifndef MANGLE_STREAM_OGRESERVER_H
#define MANGLE_STREAM_OGRESERVER_H

#include <physfs.h>

namespace Mangle {
namespace Stream {

/// A Stream wrapping a PHYSFS_file stream from the PhysFS library.
class PhysFile : public InputStream
{
  PHYSFS_file *file;

 public:
  PhysFile(PHYSFS_file *inp) : file(inp)
    {
      isSeekable = true;
      hasPosition = true;
      hasSize = true;
    }

  ~PhysFile() { PHYSFS_close(file); }

  size_t read(void *buf, size_t count)
    { return PHYSFS_read(file, buf, 1, count); }

  void seek(size_t pos) { PHYSFS_seek(file, pos); }
  size_t tell() const { return PHYSFS_tell(file); }
  size_t size() const { return PHYSFS_fileLength(file); }
  bool eof() const { return PHYSFS_eof(file); }
};

}} // namespaces
#endif
