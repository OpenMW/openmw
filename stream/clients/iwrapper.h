#ifndef MANGLE_STREAM_IWRAPPER_H
#define MANGLE_STREAM_IWRAPPER_H

#include "../stream.h"
#include <assert.h>

namespace Mangle {
namespace Stream {

/** A generic wrapper class for a Stream::Stream object.

    This is used by other implementations.
 */
class _SWrapper
{
 private:
  bool autoDel;

 protected:
  Stream *inp;

 public:
  _SWrapper(Stream *_inp, bool _autoDel = false)
    : inp(_inp), autoDel(_autoDel) { assert(inp != NULL); }

  virtual ~_SWrapper() { if(autoDel) delete inp; }
};

}} // namespaces
#endif
