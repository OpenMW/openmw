#ifndef MANGLE_STREAM_IWRAPPER_H
#define MANGLE_STREAM_IWRAPPER_H

#include "../input.h"
#include <assert.h>

namespace Mangle {
namespace Stream {

/** A generic wrapper class for a Stream::Input object.

    This is used by other implementations.
 */
class _IWrapper
{
 private:
  bool autoDel;

 protected:
  InputStream *inp;

 public:
  _IWrapper(InputStream *_inp, bool _autoDel = false)
    : inp(_inp), autoDel(_autoDel) { assert(inp != NULL); }

  virtual ~_IWrapper() { if(autoDel) delete inp; }
};

}} // namespaces
#endif
