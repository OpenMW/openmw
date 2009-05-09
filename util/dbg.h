#include <iostream>

extern "C" void dbg_trace(const char *);
extern "C" void dbg_untrace();

class MTrace
{
 public:
  MTrace(const char* str)
    { dbg_trace(str); }

  ~MTrace()
    { dbg_untrace(); }
};

#define TRACE(x) MTrace _trc(x);
