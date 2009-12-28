// This file is shared between several test programs
#include "../stream.h"
#include <assert.h>
#include <string.h>

using namespace Mangle::Stream;

// A simple dummy stream
const char _data[12] = "hello world";

class DummyInput : public Stream
{
private:
  int pos;

public:
  DummyInput() : pos(0)
  {
    isSeekable = true;
    hasPosition = true;
    hasSize = true;
  }

  size_t read(void *buf, size_t count)
  {
    assert(pos >= 0 && pos <= 11);
    if(count+pos > 11)
      count = 11-pos;
    assert(count <= 11);

    memcpy(buf, _data+pos, count);
    pos += count;

    assert(pos >= 0 && pos <= 11);
    return count;
  }

  void seek(size_t npos)
  {
    if(npos > 11) npos = 11;
    pos = npos;
  }

  size_t tell() const { return pos; }
  size_t size() const { return 11; }

  bool eof() const { return pos == 11; }
};
