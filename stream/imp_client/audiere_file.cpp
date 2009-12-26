#include "audiere_file.h"

using namespace audiere;
using namespace Mangle::Stream;

bool AudiereFile::seek(int pos, SeekMode mode)
{
  assert(inp->isSeekable);
  assert(inp->hasPosition);

  int newPos;

  switch(mode)
    {
    case BEGIN: newPos = pos; break;
    case CURRENT: newPos = pos+tell(); break;
    case END:
      // Seeking from the end. This requires that we're able to get
      // the entire size of the stream. The pos also has to be
      // non-positive.
      assert(inp->hasSize);
      assert(pos <= 0);
      newPos = inp->size() + pos;
      break;
    default:
      assert(0 && "invalid seek mode");
    }

  inp->seek(newPos);
  return inp->tell() == newPos;

}
