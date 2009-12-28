#include <iostream>

using namespace Mangle::VFS;
using namespace Mangle::Stream;
using namespace std;

void find(VFS &vfs, const std::string &file)
{
  cout << "\nFile: " << file << endl;

  if(!vfs.isFile(file))
    {
      cout << "File doesn't exist\n";
      return;
    }

  Stream *data = vfs.open(file);

  cout << "Size: " << data->size() << endl;

  char buf[13];
  buf[12] = 0;
  data->read(buf, 12);

  cout << "First 12 bytes: " << buf << "\n";
}

void testAll(VFS &vfs)
{
  find(vfs, "Makefile");     // From the file system
  find(vfs, "testfile.txt"); // From the zip
  find(vfs, "blah_bleh");    // Doesn't exist
}
