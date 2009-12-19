#include "dummy_vfs.cpp"
#include "ogre_archive.h"
#include <iostream>

using namespace Ogre;
using namespace std;

void print(StringVectorPtr lst)
{
  int s = lst->size();

  for(int i=0; i<s; i++)
    {
      cout << "  " << (*lst)[i] << endl;
    }
}

int main()
{
  VFS *vfs = new DummyVFS();
  MangleArchive arc(vfs, "dummy");

  cout << "Case: " << arc.isCaseSensitive() << endl;
  cout << "Name: " << arc.getName() << endl;
  cout << "Type: " << arc.getType() << endl;
  cout << "All files:\n";
  print(arc.list());
  cout << "Non-recursive:\n";
  print(arc.list(false, false));
  cout << "Dirs:\n";
  print(arc.list(false, true));

  DataStreamPtr file = arc.open("file1");

  cout << "filesize: " << file->size() << endl;
  cout << "contents: " << file->getAsString() << endl;

  return 0;
}
