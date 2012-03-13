#include "../servers/outfile_stream.hpp"
#include <iostream>

using namespace Mangle::Stream;
using namespace std;

void print(Stream &str)
{
  cout << "size=" << str.size()
       << " pos=" << str.tell()
       << " eof=" << str.eof()
       << endl;
}

int main()
{
  {
    cout << "\nCreating file\n";
    OutFileStream out("test.file");
    print(out);
    out.write("hello",5);
    print(out);
  }

  {
    cout << "\nAppending to file\n";
    OutFileStream out("test.file", true);
    print(out);
    out.write(" again\n",7);
    print(out);
  }

  {
    cout << "\nOverwriting file\n";
    OutFileStream out("test.file");
    print(out);
    out.write("overwrite!\n",11);
    print(out);
  }
  return 0;
}
