#include "../servers/memory_stream.hpp"
#include "../clients/ogre_datastream.hpp"
#include <iostream>

using namespace Mangle::Stream;
using namespace Ogre;
using namespace std;

int main()
{
  StreamPtr inp(new MemoryStream("hello world", 11));
  DataStreamPtr p(new Mangle2OgreStream("hello", inp));
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
