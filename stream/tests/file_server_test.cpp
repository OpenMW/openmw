#include "../servers/file_stream.h"
#include <iostream>

using namespace Mangle::Stream;
using namespace std;

int main()
{
  StreamPtr inp(new FileStream("file_server_test.cpp"));

  char buf[21];
  buf[20] = 0;
  cout << "pos=" << inp->tell() << " eof=" << inp->eof() << endl;
  inp->read(buf, 20);
  cout << "First 20 bytes: " << buf << endl;
  cout << "pos=" << inp->tell() << " eof=" << inp->eof() << endl;
  return 0;
}
