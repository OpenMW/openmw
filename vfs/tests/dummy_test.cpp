#include "dummy_vfs.cpp"

#include <iostream>
#include <string.h>

using namespace std;

void print(FileInfo &inf)
{
  cout << "name: " << inf.name << endl;
  cout << "basename: " << inf.basename << endl;
  cout << "isDir: " << inf.isDir << endl;
  cout << "size: " << inf.size << endl;
  cout << "time: " << inf.time << endl;
}
void print(FileInfoPtr inf) { print(*inf); }

void print(FileInfoList &lst)
{
  for(int i=0; i<lst.size(); i++)
    print(lst[i]);
}
void print(FileInfoListPtr lst) { print(*lst); }

int main()
{
  DummyVFS vfs;

  cout << "Listing all files:\n";
  print(vfs.list());
  cout << "\nStat for single files:\n";
  print(vfs.stat("file1"));
  cout << endl;
  print(vfs.stat("dir/file2"));
  cout << endl;
  print(vfs.stat("dir"));

  StreamPtr inp = vfs.open("file1");
  cout << "filesize: " << inp->size() << endl;

  return 0;
}
