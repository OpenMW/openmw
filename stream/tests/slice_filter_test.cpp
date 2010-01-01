#include <iostream>
#include <string.h>

#include "../filters/slice_stream.h"
#include "../servers/memory_stream.h"

using namespace Mangle::Stream;
using namespace std;

void test(StreamPtr inp)
{
  cout << "Size: " << inp->size() << endl;
  cout << "Pos: " << inp->tell() << "\nSeeking...\n";
  char data[6];
  memset(data, 0, 6);
  cout << "Reading " << inp->read(data, 6) << " bytes\n";
  cout << "Result: " << data << endl;
  cout << "Pos: " << inp->tell() << endl;
  cout << "Eof: " << inp->eof() << endl;
  inp->seek(2);
  cout << "Seeking:\nPos: " << inp->tell() << endl;
  cout << "Eof: " << inp->eof() << endl;
  cout << "Reading " << inp->read(data, 6) << " bytes\n";
  cout << "Result: " << data << endl;
  cout << "Pos: " << inp->tell() << endl;
  cout << "Eof: " << inp->eof() << endl;
  cout << "Entire stream as pointer: " << (char*)inp->getPtr() << endl;
}

int main()
{
  StreamPtr orig (new MemoryStream("hello\0world\0", 12));
  StreamPtr slice1 (new SliceStream(orig,0,6));
  StreamPtr slice2 (new SliceStream(orig,6,6));

  cout << "\nSlice 1:\n--------\n";
  test(slice1);
  cout << "\nSlice 2:\n--------\n";
  test(slice2);

  return 0;
}
