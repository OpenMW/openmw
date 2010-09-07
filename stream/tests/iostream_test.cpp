#include <iostream>
#include "../clients/io_stream.hpp"
#include "../servers/memory_stream.hpp"

using namespace Mangle::Stream;
using namespace std;

void test1()
{
  cout << "Testing ASCII reading from memory:\n";
  StreamPtr input(new MemoryStream("hello you world you", 19));
  MangleIStream inp(input);

  string str;
  while(!inp.eof())
    {
      inp >> str;
      cout << "Got: " << str << endl;
    }
}

class Dummy : public Stream
{
  int count;

public:

  Dummy() : count(0)
  {
  }

  size_t read(void *ptr, size_t num)
  {
    char *p = (char*)ptr;
    char *start = p;
    for(; (count < 2560) && (p-start < (int)num); count++)
      {
        *p = count / 10;
        p++;
      }
    return p-start;
  }

  bool eof() const { return count == 2560; }
};

void test2()
{
  cout << "\nTesting binary reading from non-memory:\n";

  StreamPtr input(new Dummy);
  MangleIStream inp(input);

  int x = 0;
  while(!inp.eof())
    {
      unsigned char buf[5];
      inp.read((char*)buf,5);

      // istream doesn't set eof() until we read _beyond_ the end of
      // the stream, so we need an extra check.
      if(inp.gcount() == 0) break;

      /*
      for(int i=0;i<5;i++)
        cout << (int)buf[i] << " ";
      cout << endl;
      */

      assert(buf[4] == buf[0]);
      assert(buf[0] == x/2);
      x++;
    }
  cout << "  Done\n";
}

struct Dummy2 : Stream
{
  Dummy2() 
  {
    isWritable = true;
    isReadable = false;
  }

  size_t write(const void *ptr, size_t num)
  {
    const char *p = (const char*)ptr;
    cout << "    Got: ";
    for(unsigned i=0;i<num;i++)
      cout << *(p++) << " ";
    cout << endl;
    return num;
  }
};

void test3()
{
  cout << "\nWriting to dummy stream:\n";

  cout << "  Pure dummy test:\n";
  StreamPtr output(new Dummy2);
  output->write("testing", 7);

  cout << "  Running through MangleOStream:\n";
  MangleOStream out(output);
  out << "hello";
  out << " - are you ok?";
  cout << "  Flushing:\n";
  out.flush();

  cout << "  Writing a hell of a lot of characters:\n";
  for(int i=0; i<127; i++)
    out << "xxxxxxxx"; // 127 * 8 = 1016
  out << "fffffff"; // +7 = 1023
  cout << "  Just one more:\n";
  out << "y";
  cout << "  And oooone more:\n";
  out << "z";

  cout << "  Flushing again:\n";
  out.flush();
  cout << "  Writing some more and exiting:\n";
  out << "blah bleh blob";
}

struct Dummy3 : Stream
{
  int pos;

  Dummy3() : pos(0)
  {
    hasPosition = true;
    isSeekable = true;
  }

  size_t read(void*, size_t num)
  {
    cout << "    Reading " << num << " bytes from " << pos << endl;
    pos += num;
    return num;
  }

  void seek(size_t npos) { pos = npos; }
  size_t tell() const { return pos; }
};

void test4()
{
  cout << "\nTesting seeking;\n";
  StreamPtr input(new Dummy3);

  cout << "  Direct reading:\n";
  input->read(0,10);
  input->read(0,5);

  MangleIStream inp(input);

  cout << "  Reading from istream:\n";
  char buf[20];
  inp.read(buf, 20);
  inp.read(buf, 20);
  inp.read(buf, 20);

  cout << "  Seeking to 30 and reading again:\n";
  inp.seekg(30);
  inp.read(buf, 20);
}

int main()
{
  test1();
  test2();
  test3();
  test4();
  return 0;
}
