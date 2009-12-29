#include "../servers/memory_stream.h"
#include "../clients/audiere_file.h"
#include <audiere.h>
#include <iostream>

using namespace Mangle::Stream;
using namespace audiere;
using namespace std;

int main()
{
  char str[12];
  memset(str, 0, 12);
  Stream *inp = new MemoryStream("hello world", 11);
  FilePtr p(new AudiereFile(inp, true));
  cout << "pos=" << p->tell() << endl;
  p->read(str, 2);
  cout << "2 bytes: " << str << endl;
  cout << "pos=" << p->tell() << endl;
  p->seek(4, File::BEGIN);
  cout << "pos=" << p->tell() << endl;
  p->read(str, 3);
  cout << "3 bytes: " << str << endl;
  p->seek(-1, File::CURRENT);
  cout << "pos=" << p->tell() << endl;
  p->seek(-4, File::END);
  cout << "pos=" << p->tell() << endl;
  p->read(str, 4);
  cout << "last 4 bytes: " << str << endl;
  p->seek(0, File::BEGIN);
  p->read(str, 11);
  cout << "entire stream: " << str << endl;
  return 0;
}
