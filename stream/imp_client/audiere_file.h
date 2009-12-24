#ifndef MANGLE_STREAM_AUDIERECLIENT_H
#define MANGLE_STREAM_AUDIERECLIENT_H

#include <audiere.h>
#include <assert.h>
#include "iwrapper.h"

namespace Mangle {
namespace Stream {

/** @brief An Audiere::File that wraps a Mangle::Stream input.

    This lets Audiere read sound files from any generic archive or
    file manager that supports Mangle streams.
 */
class AudiereFile : public audiere::RefImplementation<audiere::File>, _IWrapper
{
 public:
  AudiereFile(InputStream *inp, bool autoDel=false)
    : _IWrapper(inp, autoDel) {}

  /// Read 'count' bytes, return bytes successfully read
  int read(void *buf, int count)
    { return inp->read(buf,count); }

  /// Seek, relative to specified seek mode. Returns true if successful.
  bool seek(int pos, audiere::File::SeekMode mode);

  /// Get current position
  int tell()
    { assert(inp->hasPosition); return inp->tell(); }
};

}} // namespaces
#endif
