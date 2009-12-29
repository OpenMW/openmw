#include "../servers/memory_stream.h"
#include "ogre_datastream.h"
#include <iostream>

using namespace Mangle::Stream;
using namespace Ogre;
using namespace std;

int main()
{
  Stream *inp = new MemoryStream("hello world", 11);
  DataStreamPtr p(new MangleDataStream("hello", inp, true));
  cout << "Name: " << p->getName() << endl;
  cout << "As string: " << p->getAsString() << endl;
  cout << "pos=" << p->tell() << " eof=" << p->eof() << endl;
  p->seek(0);
  cout << "pos=" << p->tell() << " eof=" << p->eof() << endl;
  p->skip(5);
  p->skip(-2);
  cout << "pos=" << p->tell() << " eof=" << p->eof() << endl;
  return 0;
}
