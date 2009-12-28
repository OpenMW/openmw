#ifndef MANGLE_VFS_WRAPPER_H
#define MANGLE_VFS_WRAPPER_H

#include "../vfs.h"
#include <assert.h>

namespace Mangle {
namespace VFS {

/** A generic wrapper class for a VFS::VFS object.

    This is used by other implementations.
 */
class _Wrapper
{
 private:
  bool autoDel;

 protected:
  VFS *vfs;

 public:
  _Wrapper(VFS *_vfs, bool _autoDel = false)
    : vfs(_vfs), autoDel(_autoDel) { assert(vfs != NULL); }

  virtual ~_Wrapper() { if(autoDel) delete vfs; }
};

}} // namespaces
#endif
