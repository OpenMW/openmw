#include "audiere_file.h"

using namespace audiere;
using namespace Mangle::Stream;

bool AudiereFile::seek(int pos, SeekMode mode)
{
  assert(inp->isSeekable);
  assert(inp->hasPosition);

  if(mode == BEGIN)
    {
      // Absolute position
      inp->seek(pos);
      return inp->tell() == pos;
    }
  if(mode == CURRENT)
    {
      // Current position
      int cpos = inp->tell();

      // Seek to a elative position
      inp->seek(cpos + pos);
      return inp->tell() == (pos+cpos);
    }
  if(mode == END)
    {
      // Seeking from the end. This requires that we're able to get
      // the entire size of the file. The pos also has to be
      // non-positive.
      assert(inp->hasSize);
      assert(pos <= 0);

      size_t epos = inp->size();
      inp->seek(epos + pos);
      return inp->tell() == (epos+pos);
    }
  assert(0 && "invalid seek mode");
}
